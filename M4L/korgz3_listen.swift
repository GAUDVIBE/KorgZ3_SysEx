// Ecoute un port CoreMIDI et sort sur stdout chaque dump Korg Z3 :
//   DUMP <octets en decimal, F0..F7 inclus>
// Filtre : F0 42 30 1D 40 ... F7, longueur max 96 (cf. firmware Arduino).
import CoreMIDI
import Foundation

let wanted = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "USB MIDI Interface"
let HEADER: [UInt8] = [0xF0, 0x42, 0x30, 0x1D, 0x40]
var buf = [UInt8](); var inSysex = false

func name(_ o: MIDIObjectRef) -> String {
    var p: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(o, kMIDIPropertyDisplayName, &p)
    return p?.takeRetainedValue() as String? ?? ""
}

func emit(_ m: [UInt8]) {
    guard m.count >= HEADER.count + 1, m.count <= 96,
          Array(m.prefix(HEADER.count)) == HEADER else { return }
    print("DUMP " + m.map { String($0) }.joined(separator: " "))
    fflush(stdout)
}

func feed(_ b: UInt8) {
    if b == 0xF0 { inSysex = true; buf = [b] }
    else if inSysex {
        buf.append(b)
        if b == 0xF7 { inSysex = false; emit(buf); buf = [] }
        else if buf.count > 200 { inSysex = false; buf = [] }
    }
}

var client = MIDIClientRef(), port = MIDIPortRef()
MIDIClientCreate("z3listen" as CFString, nil, nil, &client)
MIDIInputPortCreateWithBlock(client, "in" as CFString, &port) { listPtr, _ in
    for packet in listPtr.unsafeSequence() {
        for byte in packet.bytes() { feed(byte) }
    }
}
var connected = 0
for i in 0..<MIDIGetNumberOfSources() {
    let s = MIDIGetSource(i)
    if name(s).contains(wanted) { MIDIPortConnectSource(port, s, nil); connected += 1
        FileHandle.standardError.write("connecte a \(name(s))\n".data(using: .utf8)!) }
}
if connected == 0 {
    FileHandle.standardError.write("aucune source ne correspond a \"\(wanted)\"\n".data(using: .utf8)!)
    exit(1)
}
CFRunLoopRun()
