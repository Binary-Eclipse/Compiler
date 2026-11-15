#!/usr/bin/env python3
"""Convert an old Python-style ShakilScript file to the new ShakilScript names.
Usage: python3 convert_to_ss.py in.ss > out.ss   (or: --inplace file1 file2 ...)"""
import re, sys
MAP = {"print":"show","input":"ask","if":"when","elif":"orwhen","else":"otherwise","for":"each",
 "while":"during","range":"span","break":"stop","continue":"skip","return":"give","def":"task",
 "sort":"arrange","sorted":"arranged","in":"within","index":"locate","find":"seek","append":"push",
 "remove":"erase","pop":"pull","len":"size","count":"tally","reverse":"flip","int":"whole",
 "float":"decimal","str":"text","bool":"truth"}
TOK = re.compile(r'(#[^\n]*)|("(?:[^"\\\n]|\\.)*"?|\'(?:[^\'\\\n]|\\.)*\'?)|([A-Za-z_]\w*)')
def convert(src, name="<input>"):
    new_words = set(MAP.values())
    def rep(m):
        if m.group(3):
            w = m.group(3)
            if w in new_words:
                print(f"warning: {name}: variable '{w}' is now a reserved ShakilScript word - rename it", file=sys.stderr)
            return MAP.get(w, w)
        return m.group(0)
    return TOK.sub(rep, src)
if __name__ == "__main__":
    a = sys.argv[1:]
    if a and a[0] == "--inplace":
        for f in a[1:]:
            t = open(f, encoding="utf-8").read(); open(f, "w", encoding="utf-8", newline="\n").write(convert(t, f))
    else:
        sys.stdout.write(convert(open(a[0], encoding="utf-8").read(), a[0]))
