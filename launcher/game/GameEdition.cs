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
        public static string LaunchArguments(string edition) {
            // Pass only validated, fixed tokens to Warcraft, never UI text or a saved arbitrary argument.
            return Normalize(edition)==ReignOfChaos ? "-opengl -classic" : "-opengl";
        }
    }
}
