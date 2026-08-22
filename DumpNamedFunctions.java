// Decompile every function that carries a real (exported) name, grouped by
// engine subsystem prefix. Skips compiler/CRT and auto-generated names.
//@category RCCars

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public class DumpNamedFunctions extends GhidraScript {

    private static boolean isAuto(String n) {
        return n.startsWith("FUN_") || n.startsWith("thunk_") || n.startsWith("Ordinal_")
            || n.startsWith("_") || n.startsWith("entry") || n.startsWith("switchD")
            || n.startsWith("caseD") || n.startsWith("__");
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File od = new File(args.length > 0 ? args[0] : "/tmp");
        od.mkdirs();
        String prog = currentProgram.getName();

        List<Function> named = new ArrayList<>();
        FunctionIterator fit = currentProgram.getFunctionManager().getFunctions(true);
        while (fit.hasNext()) {
            Function f = fit.next();
            if (!isAuto(f.getName())) named.add(f);
        }
        named.sort(Comparator.comparing(Function::getName));
        println(prog + ": named functions = " + named.size());

        DecompInterface di = new DecompInterface();
        if (!di.openProgram(currentProgram)) {
            println("decompiler open failed: " + di.getLastMessage());
            return;
        }

        PrintWriter w = new PrintWriter(new File(od, prog + ".named.c"));
        PrintWriter idx = new PrintWriter(new File(od, prog + ".named_index.txt"));
        int ok = 0, fail = 0;
        for (Function f : named) {
            if (monitor.isCancelled()) break;
            idx.printf("%-40s %s  size=%d  callers=%d%n", f.getName(), f.getEntryPoint(),
                       f.getBody().getNumAddresses(), f.getCallingFunctions(monitor).size());
            w.println("/* ===== " + f.getName() + " @ " + f.getEntryPoint()
                      + "  size=" + f.getBody().getNumAddresses() + " ===== */");
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            if (res != null && res.decompileCompleted()) {
                w.println(res.getDecompiledFunction().getC());
                ok++;
            } else {
                w.println("/* DECOMPILE FAILED: "
                          + (res == null ? "null" : res.getErrorMessage()) + " */");
                fail++;
            }
            w.println();
        }
        w.close();
        idx.close();
        di.dispose();
        println(prog + ": decompiled " + ok + " ok, " + fail + " failed");
    }
}
