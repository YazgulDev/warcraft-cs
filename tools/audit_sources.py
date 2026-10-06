"""Reject non-source content and likely credentials before a commit or source release."""
from __future__ import annotations
import argparse
import re
import subprocess
from pathlib import Path, PurePosixPath

ROOT=Path(__file__).resolve().parents[1]

ROOT_FILES = {'README.md','INSTALL.md','TROUBLESHOOTING.md','NOTICE','LICENSE-MIT','LICENSE-APACHE',
              'AGENTS.md','CONTRIBUTING.md','CHANGELOG.md','VERSION','requirements.txt','.gitignore',
              '.gitattributes','setup.cmd','play.cmd'}
ROOTS = {'src','tests','tools','setup','config','skills','licenses','docs','launcher'}
EXTENSIONS = {'.cpp','.hpp','.inc','.py','.ps1','.ini','.md','.yaml','.txt','.cs'}
RULES = {
    'GitHub credential': re.compile(rb'\bgh[opusr]_[A-Za-z0-9]{30,}\b|\bgithub_pat_[A-Za-z0-9_]{30,}\b'),
    'API credential': re.compile(rb'\bsk-(?:proj-)?[A-Za-z0-9_-]{25,}|\bAKIA[A-Z0-9]{16}\b'),
    'private key': re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
    'credential assignment': re.compile(rb'(?im)^\s*(?:FAL_KEY|GH_TOKEN|GITHUB_TOKEN|OPENAI_API_KEY|ANTHROPIC_API_KEY)\s*=\s*[\x22\x27]?[A-Za-z0-9_-]{16,}'),
    'private owner path': re.compile(rb'(?i)[A-Z]:[\\/]Users[\\/]|F:[\\/]Games[\\/]gameMesh'),
    'decompiler dump': re.compile(rb'(?im)^\s*(?://|#|/\*)?\s*decompiled[ \t]+with|\b(?:FUN|DAT|sub)_[0-9a-f]{6,}\b'),
}

def git(*args: str) -> bytes:
    # Scope Git's ownership allowance to this explicit project, including managed Windows sandbox accounts.
    result = subprocess.run(['git','-c',f'safe.directory={ROOT.as_posix()}',*args],cwd=ROOT,
                            check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    return result.stdout

def inspect(name: str, data: bytes) -> list[str]:
    path=PurePosixPath(name)
    allowed=name in ROOT_FILES or (path.parts[0] in ROOTS and path.suffix in EXTENSIONS)
    failures=[]
    if not allowed or any(p in {'.local','build','vendor','third_party','assets','__pycache__'} for p in path.parts):
        failures.append('outside source-only allowlist')
    if len(data)>256_000: failures.append('oversized source file')
    if b'\x00' in data: failures.append('binary content')
    try: data.decode('utf-8-sig')
    except UnicodeDecodeError: failures.append('non-UTF-8 content')
    # Diagnostics name the rule and file, never print matching secrets.
    failures.extend(label for label,pattern in RULES.items() if pattern.search(data))
    return failures

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--staged',action='store_true')
    parser.add_argument('--revision',default='HEAD')
    args=parser.parse_args()
    if args.staged:
        # Inspect the complete index tree, not merely changed filenames, so prior tracked artifacts cannot slip through.
        names=git('ls-files','-z').decode().split('\0')
    else: names=git('ls-tree','-r','--name-only','-z',args.revision).decode().split('\0')
    errors=0;count=0
    for name in filter(None,names):
        count+=1;data=git('show',(':' if args.staged else args.revision+':')+name)
        for rule in inspect(name,data):
            print(f'FAIL {name}: {rule}');errors+=1
    if errors: return 1
    print(f'PASS: {count} source-only files; no detected credentials, private paths, binary content or decompiler dumps.')
    return 0

if __name__=='__main__': raise SystemExit(main())
