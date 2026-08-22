// Report analysis-option state and basic program facts.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.framework.options.Options;

public class ProjInfo extends GhidraScript {
    @Override
    public void run() throws Exception {
        println("PROGRAM: " + currentProgram.getName());
        println("FUNCS: " + currentProgram.getFunctionManager().getFunctionCount());
        Options o = currentProgram.getOptions(Program.ANALYSIS_PROPERTIES);
        for (String n : o.getOptionNames()) {
            if (n.toLowerCase().contains("param") || n.toLowerCase().contains("decompil")) {
                println("OPT " + n + " = " + o.getValueAsString(n));
            }
        }
        Function f = getFunctionAt(toAddr(0x4f0910L));
        println("4f0910 = " + (f == null ? "UNDEFINED" : f.getName() + " params=" + f.getParameterCount()));
        f = getFunctionAt(toAddr(0x4f5400L));
        println("4f5400 = " + (f == null ? "UNDEFINED" : f.getName()));
        f = getFunctionAt(toAddr(0x4eea50L));
        println("4eea50 = " + (f == null ? "UNDEFINED" : f.getName() + " params=" + f.getParameterCount()));
    }
}
