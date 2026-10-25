// Synthetic sound-library ABI: no retail code/data, with aliases and an ordinal-only entry.
extern "C" int __stdcall Named(){return 42;}
extern "C" int __stdcall Hidden(){return 77;}
extern "C" int __stdcall Decorated(){return 99;}
