using System;

namespace WarcraftCSLauncher {
    // Both EXE variants share the edition contract; legacy preferences retain Frozen Throne.
    public static class GameEdition {
        public const string FrozenThrone="FrozenThrone";
        public const string ReignOfChaos="ReignOfChaos";
        public static string Normalize(string edition) {
            if (String.IsNullOrEmpty(edition)) return FrozenThrone;
            if (edition!=FrozenThrone && edition!=ReignOfChaos)
                throw new InvalidOperationException("Choose Frozen Throne or Reign of Chaos.");
            return edition;
        }
        public static string LaunchArguments(string edition,bool direct3D9=false) {
            // Pass only validated, fixed tokens to Warcraft, never UI text or a saved arbitrary argument.
            // 1.27a's automatic renderer uses D3D9; edition selection remains independent of graphics.
            return ((direct3D9 ? "" : "-opengl")+(Normalize(edition)==ReignOfChaos ? " -classic" : "")).Trim();
        }
    }
}
