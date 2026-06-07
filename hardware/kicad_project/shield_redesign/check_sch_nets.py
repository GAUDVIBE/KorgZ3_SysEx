import re, pathlib

sch = pathlib.Path("SysEx_Patcher.kicad_sch").read_text()

# Nets D22..D27 : présents comme net-labels (label "DXX" ...)
for n in range(22, 28):
    assert re.search(r'\(label "D%d"' % n, sch), "net-label D%d absent du schema" % n

# 3 nouveaux boutons SW6 / SW7 / SW8
for ref in ["SW6", "SW7", "SW8"]:
    assert re.search(r'"Reference" "%s"' % ref, sch), "%s absent" % ref

# 3 résistances de limitation LED
for ref in ["R28", "R29", "R30"]:
    assert re.search(r'"Reference" "%s"' % ref, sch), "%s absent" % ref

# 3 nouvelles LEDs D7 / D8 / D9
for ref in ["D7", "D8", "D9"]:
    assert re.search(r'"Reference" "%s"' % ref, sch), "LED %s absente" % ref

print("check_sch_nets OK — D22..D27 | SW6/SW7/SW8 | R28/R29/R30 | D7/D8/D9")
