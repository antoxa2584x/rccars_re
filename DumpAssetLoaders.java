// Inventory functions, then locate + decompile the code that references
// asset-format magic strings (pack container, scene DB, CSI textures).
//@category RCCars

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Reference;

import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class DumpAssetLoaders extends GhidraScript {

    // Substrings that should sit next to asset-format parsing code.
    private static final String[] NEEDLES = {
        "CREAT Studio", "Scene Project", ".pack", ".sb", ".csi", ".crs",
        "MOD_", "csi", "Texture", "PackFile", "Database", "CacheData",
        "Path", "Smash", ".dat", ".gm"
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File od = new File(args.length > 0 ? args[0] : "/tmp");
        od.mkdirs();
        String prog = currentProgram.getName();

        // --- 1. full function inventory -------------------------------
        int nf = 0;
        PrintWriter fw = new PrintWriter(new File(od, prog + ".functions.csv"));
        fw.println("address,name,size,params,calledBy");
        FunctionIterator fit = currentProgram.getFunctionManager().getFunctions(true);
        while (fit.hasNext()) {
            Function f = fit.next();
            fw.printf("%s,%s,%d,%d,%d%n",
                f.getEntryPoint(), f.getName(),
                f.getBody().getNumAddresses(),
                f.getParameterCount(),
                f.getCallingFunctions(monitor).size());
            nf++;
        }
        fw.close();
        println(prog + ": functions = " + nf);

        // --- 2. magic strings -> referencing functions ----------------
        Map<Function, Set<String>> hits = new LinkedHashMap<>();
        PrintWriter sw = new PrintWriter(new File(od, prog + ".strings_xref.txt"));
        int nstr = 0;

        DataIterator dit = currentProgram.getListing().getDefinedData(true);
        while (dit.hasNext()) {
            Data d = dit.next();
            Object v = d.getValue();
            if (!(v instanceof String)) continue;
            String s = (String) v;
            if (s.length() < 3) continue;
            nstr++;

            boolean match = false;
            for (String n : NEEDLES) {
                if (s.contains(n)) { match = true; break; }
            }
            if (!match) continue;

            List<String> refs = new ArrayList<>();
            for (Reference r : getReferencesTo(d.getAddress())) {
                Address from = r.getFromAddress();
                Function f = getFunctionContaining(from);
                if (f == null) { refs.add(from + " (no func)"); continue; }
                refs.add(from + " in " + f.getName() + " @" + f.getEntryPoint());
                hits.computeIfAbsent(f, k -> new LinkedHashSet<>()).add(s);
            }
            if (!refs.isEmpty()) {
                sw.println("STRING " + d.getAddress() + "  " + s.replace("\n", "\\n"));
                for (String r : refs) sw.println("    <- " + r);
                sw.println();
            }
        }
        sw.close();
        println(prog + ": defined strings = " + nstr + ", candidate funcs = " + hits.size());

        // --- 3. decompile the candidates ------------------------------
        DecompInterface di = new DecompInterface();
        if (!di.openProgram(currentProgram)) {
            println("decompiler failed to open: " + di.getLastMessage());
            return;
        }
        PrintWriter cw = new PrintWriter(new File(od, prog + ".candidates.c"));
        int ok = 0;
        for (Map.Entry<Function, Set<String>> e : hits.entrySet()) {
            Function f = e.getKey();
            cw.println("/* ===== " + f.getName() + " @ " + f.getEntryPoint()
                       + "  size=" + f.getBody().getNumAddresses() + " =====");
            for (String s : e.getValue()) cw.println(" *   ref: " + s.replace("\n", "\\n"));
            cw.println(" */");
            DecompileResults res = di.decompileFunction(f, 90, monitor);
            if (res != null && res.decompileCompleted()) {
                cw.println(res.getDecompiledFunction().getC());
                ok++;
            } else {
                cw.println("/* DECOMPILE FAILED: "
                           + (res == null ? "null" : res.getErrorMessage()) + " */");
            }
            cw.println();
        }
        cw.close();
        di.dispose();
        println(prog + ": decompiled " + ok + "/" + hits.size() + " candidates");
    }
}
