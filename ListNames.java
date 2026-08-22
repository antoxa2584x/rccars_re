// Dump every function as "address name size" to a file. //@category RCCars
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class ListNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] av = getScriptArgs();
        java.io.PrintWriter out = new java.io.PrintWriter(new java.io.FileWriter(av[0]));
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            out.println(f.getEntryPoint() + " " + f.getName() + " " + f.getBody().getNumAddresses());
        }
        out.flush(); out.close();
    }
}
