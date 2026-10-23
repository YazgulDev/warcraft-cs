using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace WarcraftCS.Setup {
    // Rebuild only an owned loader's reserved export data; executable code and retail libraries stay untouched.
    public static class MilesForwarderBuilder {
        private const int Capacity=131072;
        private sealed class Image {
            public readonly byte[] Data;
            public readonly int Optional,Sections,Count;
            public Image(byte[] data) {
                Data=data;Range(0,64);
                if(U16(0)!=0x5a4d)throw new InvalidDataException("Missing DOS header.");
                int pe=Index(U32(60));Range(pe,24);
                if(U32(pe)!=0x4550 || U16(pe+4)!=0x14c || (U16(pe+22)&0x2000)==0)
                    throw new InvalidDataException("An x86 DLL is required.");
                Optional=pe+24;int length=U16(pe+20);Range(Optional,length);
                if(length<224 || U16(Optional)!=0x10b || U32(Optional+92)<16)
                    throw new InvalidDataException("Unsupported PE optional header.");
                Count=U16(pe+6);Sections=Optional+length;
                if(Count<1 || Count>96)throw new InvalidDataException("Invalid section count.");
                Range(Sections,Count*40);
                for(int i=0;i<Count;++i) {
                    int section=Sections+i*40;
                    Range(Index(U32(section+20)),Index(U32(section+16)));
                }
            }
            public void Range(int offset,int count) {
                if(offset<0 || count<0 || offset>Data.Length-count)throw new InvalidDataException("Truncated PE data.");
            }
            public static int Index(uint value) {
                if(value>int.MaxValue)throw new InvalidDataException("PE offset is too large.");return (int)value;
            }
            public ushort U16(int at){Range(at,2);return BitConverter.ToUInt16(Data,at);}
            public uint U32(int at){Range(at,4);return BitConverter.ToUInt32(Data,at);}
            public int Offset(uint rva,int count) {
                for(int i=0;i<Count;++i) {
                    int section=Sections+i*40;uint start=U32(section+12),raw=U32(section+16);
                    if(rva>=start && (ulong)(rva-start)+(ulong)count<=raw) {
                        ulong result=(ulong)U32(section+20)+rva-start;
                        if(result>int.MaxValue)break;Range((int)result,count);return (int)result;
                    }
                }
                throw new InvalidDataException("Unmapped PE export RVA.");
            }
            public string Name(uint rva) {
                var bytes=new List<byte>();
                for(uint i=0;i<1024;++i) {
                    if(rva>uint.MaxValue-i)break;
                    byte value=Data[Offset(rva+i,1)];if(value==0)return Encoding.ASCII.GetString(bytes.ToArray());
                    if(value<33 || value>126)throw new InvalidDataException("Non-ASCII export name.");
                    bytes.Add(value);
                }
                throw new InvalidDataException("Unterminated export name.");
            }
        }
        private static void Put(byte[] bytes,int at,uint value){Buffer.BlockCopy(BitConverter.GetBytes(value),0,bytes,at,4);}
        private static int Space(ref int cursor,int count) {
            if(count<0 || cursor>Capacity-count)throw new InvalidDataException("Miles export table exceeds loader capacity.");
            int result=cursor;cursor+=count;return result;
        }
        private static uint Text(byte[] block,uint start,ref int cursor,string value) {
            byte[] bytes=Encoding.ASCII.GetBytes(value+"\0");int at=Space(ref cursor,bytes.Length);
            Buffer.BlockCopy(bytes,0,block,at,bytes.Length);return start+(uint)at;
        }
        public static byte[] Build(byte[] template,byte[] original) {
            var loader=new Image(template);var sound=new Image(original);
            // A package checksum is verified by the caller; this additionally confines all edits to a dedicated data section.
            int reserved=-1;
            for(int i=0;i<loader.Count;++i) {
                int section=loader.Sections+i*40;
                if(Encoding.ASCII.GetString(template,section,8).TrimEnd('\0')!=".wcsfwd")continue;
                if(reserved>=0 || loader.U32(section+8)!=Capacity || loader.U32(section+16)<Capacity ||
                    (loader.U32(section+36)&0x20000000)!=0 || (loader.U32(section+36)&0x40000000)==0)
                    throw new InvalidDataException("Invalid Miles loader export reservation.");
                reserved=section;
            }
            if(reserved<0 || loader.U32(loader.Optional+128)!=0)
                throw new InvalidDataException("An unsigned adaptive Miles loader template is required.");
            uint exportRva=sound.U32(sound.Optional+96);
            if(exportRva==0 || sound.U32(sound.Optional+100)<40)throw new InvalidDataException("Miles DLL has no export table.");
            int directory=sound.Offset(exportRva,40);
            uint ordinalBase=sound.U32(directory+16),functionCount=sound.U32(directory+20),nameCount=sound.U32(directory+24);
            if(ordinalBase<1 || functionCount<1 || functionCount>4096 || nameCount>8192 || (ulong)ordinalBase+functionCount>65536)
                throw new InvalidDataException("Unsupported Miles export counts.");
            int functions=sound.Offset(sound.U32(directory+28),(int)functionCount*4);
            int names=nameCount==0 ? 0 : sound.Offset(sound.U32(directory+32),(int)nameCount*4);
            int ordinals=nameCount==0 ? 0 : sound.Offset(sound.U32(directory+36),(int)nameCount*2);
            var labels=new List<KeyValuePair<string,ushort>>();var unique=new HashSet<string>(StringComparer.Ordinal);
            for(int i=0;i<(int)nameCount;++i) {
                ushort index=sound.U16(ordinals+i*2);string name=sound.Name(sound.U32(names+i*4));
                if(index>=functionCount || sound.U32(functions+index*4)==0 || name.Length==0 || !unique.Add(name))
                    throw new InvalidDataException("Invalid Miles named export.");
                labels.Add(new KeyValuePair<string,ushort>(name,index));
            }
            labels.Sort((a,b)=>StringComparer.Ordinal.Compare(a.Key,b.Key));
            uint rva=loader.U32(reserved+12);int file=Image.Index(loader.U32(reserved+20));
            if(rva>uint.MaxValue-Capacity)throw new InvalidDataException("Invalid loader section RVA.");
            var block=new byte[Capacity];int cursor=40;
            int addressTable=Space(ref cursor,(int)functionCount*4),nameTable=Space(ref cursor,labels.Count*4),ordinalTable=Space(ref cursor,labels.Count*2);
            Put(block,12,Text(block,rva,ref cursor,"Mss32.dll"));Put(block,16,ordinalBase);
            Put(block,20,functionCount);Put(block,24,(uint)labels.Count);
            Put(block,28,rva+(uint)addressTable);Put(block,32,rva+(uint)nameTable);Put(block,36,rva+(uint)ordinalTable);
            // Forward every occupied ordinal exactly; aliases, decorated names and holes retain their original identities.
            for(int i=0;i<(int)functionCount;++i)if(sound.U32(functions+i*4)!=0)
                Put(block,addressTable+i*4,Text(block,rva,ref cursor,"WarcraftOriginalMss.#"+(ordinalBase+(uint)i).ToString(CultureInfo.InvariantCulture)));
            for(int i=0;i<labels.Count;++i) {
                Put(block,nameTable+i*4,Text(block,rva,ref cursor,labels[i].Key));
                Buffer.BlockCopy(BitConverter.GetBytes(labels[i].Value),0,block,ordinalTable+i*2,2);
            }
            byte[] output=(byte[])template.Clone();Buffer.BlockCopy(block,0,output,file,Capacity);
            Put(output,loader.Optional+96,rva);Put(output,loader.Optional+100,(uint)cursor);Put(output,loader.Optional+64,0);
            return output;
        }
    }
}
