import re, pathlib
sch = pathlib.Path("SysEx_Patcher.kicad_sch").read_text()
for n in range(22,28):
    assert re.search(r'"D%d"'%n, sch), "net D%d absent du schema"%n
for ref in ["SW5","SW6","SW7","R28","R29","R30"]:
    assert re.search(r'"Reference" "%s"'%ref, sch), "%s absent"%ref
print("check_sch_nets OK — D22..D27 + SW5-7 + R28-30 presents")
