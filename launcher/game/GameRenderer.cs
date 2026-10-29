using System;
using System.IO;

namespace WarcraftCSLauncher {
    // Renderer policy is independent of campaign edition and applies only to the next game process.
    public static class GameRenderer {
        public const string Auto="Auto", OpenGL="OpenGL", Direct3D="Direct3D";
        public static string Normalize(string renderer) {
            if(String.IsNullOrEmpty(renderer))return Auto;
            if(renderer!=Auto && renderer!=OpenGL && renderer!=Direct3D)
                throw new InvalidOperationException("Choose Auto, OpenGL or Direct3D.");
            return renderer;
        }
        public static bool UsesDirect3D(string renderer,bool host127) {
            string selected=Normalize(renderer);
            return selected==Direct3D || (selected==Auto && host127);
        }
        public static void RequireRuntime(string runtime,bool direct3D,bool host127) {
            // 1.26a's native D3D8 cannot display our D3D9 overlay without the app-local bridge.
            if(!direct3D || host127)return;
            foreach(string name in new[] {"d3d8.dll","d3dx9_43.dll","d3dcompiler_43.dll","direct3d-runtime.json"})
                if(!File.Exists(Path.Combine(runtime,name)))
                    throw new InvalidOperationException("Direct3D components are missing. Run Install or Update with this launcher; setup prepares them automatically.");
        }
    }
}
