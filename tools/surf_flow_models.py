"""Original closed bank-transition pieces; adjacent endpoints share the exact profile."""
from surf_models import model, checker, prism, destructables
from surf_flow_layout import HALF_PAD, HALF_RAIL, spans


def solid(span):
    # Flat -> A-frame -> flat transitions permit continuous movement onto turning decks.
    sections=[]
    for x,ridge,bank in ((-span.length/2,-span.drop/2,span.bank0),(span.length/2,span.drop/2,span.bank1)):
        root=ridge-bank
        sections.append([(x,-HALF_RAIL,root-96),(x,HALF_RAIL,root-96),
                         (x,HALF_RAIL,root),(x,0,ridge),(x,-HALF_RAIL,root)])
    left,right=sections
    result=[]
    for i in range(1,4):
        result.extend(((left[0],left[i+1],left[i]),(right[0],right[i],right[i+1])))
    for i in range(5):
        j=(i+1)%5
        result.extend(((left[i],left[j],right[j]),(left[i],right[j],right[i])))
    return result


def definitions():
    return [('FP01','FlowPad'),('FP02','FlowFinish')]+[(f'SF{i:02}',f'Flow{i:02}') for i,_ in enumerate(spans())]


def assets():
    prefix='WarcraftCS/Surf/'
    result={prefix+name+'.tga':checker(rgb) for name,rgb in (
        ('FlowBlue',(55,160,245)),('FlowOrange',(245,135,45)),
        ('FlowGreen',(55,210,125)),('FlowGold',(235,210,65)))}
    pad=prism(HALF_PAD*2,[(-HALF_PAD,-96),(HALF_PAD,-96),(HALF_PAD,0),(-HALF_PAD,0)])
    for name,color in (('FlowPad','FlowGreen'),('FlowFinish','FlowGold')):
        result[prefix+name+'.mdx']=model(pad,(prefix+color+'.tga').replace('/','\\'),name)
    for i,span in enumerate(spans()):
        texture='FlowOrange' if span.stage==2 else 'FlowBlue'
        result[prefix+f'Flow{i:02}.mdx']=model(solid(span),(prefix+texture+'.tga').replace('/','\\'),f'Flow {i:02}')
    return result


def objects():
    return destructables(definitions())
