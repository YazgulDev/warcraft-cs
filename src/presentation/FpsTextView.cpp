#include "GraphicsApi.hpp"
#include "FpsTextView.hpp"
#include "HudRenderState.hpp"
#include <vector>
#include <algorithm>

void FpsTextView::Draw(const FpsTextEntry& entry,const std::string& lastMessage,bool showLast,int width,int height,HudFont& font){
    // The shared HUD scope restores inherited raster state in OpenGL and the Direct3D adapter.
    if(!entry.Opened() && !showLast)return;
    HudRenderState state;GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();font.Ensure(20);
    int columns=std::max(8,(width-64)/std::max(1,font.Advance()));
    std::string line=entry.Opened() ? entry.Buffer()+"_" : lastMessage;
    std::vector<std::string> rows;std::string row;int count=0;
    for(auto p=line.c_str();*p;){auto c=UnicodeText::Next(p);UnicodeText::Append(row,c);if(++count>=columns){rows.push_back(row);row.clear();count=0;}}
    if(!row.empty())rows.push_back(row);if(rows.size()>4)rows.erase(rows.begin(),rows.end()-4);
    float y=height*.58f;
    glDisable(GL_TEXTURE_2D);glColor4f(.02f,.025f,.03f,.92f);glBegin(GL_QUADS);
    glVertex2f(18,y-26);glVertex2f(float(width-18),y-26);glVertex2f(float(width-18),y+40+rows.size()*24);glVertex2f(18,y+40+rows.size()*24);glEnd();
    glColor4f(1,.8f,.3f,1);font.Draw(32,y,entry.Opened() ? "CHAT | ENTER: SEND  ESC: CANCEL" : "YOU:");
    glColor4f(1,1,1,1);for(const auto& text:rows){y+=24;font.Draw(32,y,text.c_str());}
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);
}
