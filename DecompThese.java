// Decompile a list of addresses (getScriptArgs) to stdout.
// Creates a function first if the address is undefined.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.ConsoleTaskMonitor;

public class DecompThese extends GhidraScript {
    java.io.PrintWriter out;
    @Override
    public void run() throws Exception {
        String[] av = getScriptArgs();
        out = new java.io.PrintWriter(new java.io.FileWriter(av[0]));
        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        opts.setMaxWidth(110);
        di.setOptions(opts);
        di.toggleCCode(true);
        di.toggleSyntaxTree(true);
        di.setSimplificationStyle("decompile");
        if (!di.openProgram(currentProgram)) {
            println("ERR: openProgram failed: " + di.getLastMessage());
            return;
        }
        for (int ai = 1; ai < av.length; ai++) {
            String s = av[ai];
            Address a = toAddr(Long.parseLong(s, 16));
            Function f = getFunctionContaining(a);
            if (f == null) {
                f = createFunction(a, null);
                println("// created function at " + a);
            }
            if (f == null) {
                println("// FAILED to create function at " + a);
                continue;
            }
            StringBuilder cb = new StringBuilder();
            for (Reference r : getReferencesTo(f.getEntryPoint())) {
                Function c = getFunctionContaining(r.getFromAddress());
                if (c != null) cb.append(c.getName()).append(" ");
            }
            out.println("/* ===== " + f.getName() + " @ " + f.getEntryPoint()
                    + "  size=" + f.getBody().getNumAddresses()
                    + "  callers: " + cb + " ===== */");
            DecompileResults res = di.decompileFunction(f, 120, new ConsoleTaskMonitor());
            if (res.decompileCompleted() && res.getDecompiledFunction() != null) {
                out.println(res.getDecompiledFunction().getC());
            } else {
                out.println("// DECOMPILE FAILED: " + res.getErrorMessage());
            }
        }
        out.flush(); out.close();
        di.dispose();
    }
}
