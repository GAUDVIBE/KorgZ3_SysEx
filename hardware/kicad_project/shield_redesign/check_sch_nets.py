import re, pathlib

sch = pathlib.Path("SysEx_Patcher.kicad_sch").read_text()

# Nets D14..D19 : présents comme net-labels (label "DXX" ...)
for n in range(14, 20):
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

# --- v1.1 : mini-XLR 5 points et canal switch ---
assert re.search(r'Conn_01x05', sch), "J10 n'est pas un connecteur 5 points"
assert re.search(r'MiniXLR-5_Switchcraft_TRAPC_Horizontal', sch), \
    "empreinte mini-XLR absente"
for ref in ["R31", "R32", "C20"]:
    assert re.search(r'"Reference" "%s"' % ref, sch), \
        "%s absent (filtre / pull-down du canal switch)" % ref
assert re.search(r'\(label "SW_BEND"', sch), "net-label SW_BEND absent"

print("check_sch_nets OK — D14..D19 | SW6/SW7/SW8 | R28/R29/R30 | D7/D8/D9 | "
      "J10 mini-XLR 5pts | R31/R32/C20 | SW_BEND")
