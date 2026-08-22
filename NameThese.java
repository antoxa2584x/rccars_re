// Apply names + plate comments to the recovered physics functions.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class NameThese extends GhidraScript {
    static final String[][] N = {
        // --- ODE / integrator ---
        {"4f5e50","carPhysTick","Per-frame vehicle update. Up to 4 substeps of the ODE."},
        {"4f5b60","carSubstepCCD","Conservative advancement: cap dt so no collision sphere moves >0.9r."},
        {"4f0270","carSubstepContact","Time-of-impact bisection (0.8/0.5 blend, <=5 tries)."},
        {"4f5550","rbOdeStep","Thunk to rbEulerStep."},
        {"4f5590","rbEulerStep","EXPLICIT EULER: yout[i]=y[i]+dt*ydot[i] for i<32. The integrator."},
        {"4f5400","carStateDeriv","ydot = (v, 0.5*(0,w)(x)q, F, tau); indices 13..31 zeroed."},
        {"4f5180","carGetState","body -> y[32]: x,q,P,L then 3x6 wheel scalars + aux."},
        {"4f5290","carSetState","y[32] -> body; normalise q, v=P/m, rebuild Iinv, w=Iinv*L."},
        {"4f5df0","collSpheresMoved","Compare two collision-sphere sets for excessive motion."},
        // --- rigid body core (engine layer, 0x474xxx-0x476xxx) ---
        {"475100","rbSumForcesTorques","F = sum f; tau = sum (p-x) x f. Overwrites accumulators."},
        {"4749d0","rbTorqueToCouple","Pure torque -> two opposed forces at +-r, r perp to tau."},
        {"474e00","rbUpdateInvInertiaWorld","Iinv_world = M' * Ibodyinv * M (row-vector convention)."},
        {"474910","rbMat3MulVec3","3x3 block of a row-stride-4 matrix times vec3. Used for w=Iinv*L."},
        {"474ed0","rbPointVelocity","vp = v + w x (p - x)."},
        // --- math helpers ---
        {"4079f0","quatMul","Hamilton product, (w,x,y,z)."},
        {"407990","quatScale",""},
        {"4078d0","quatNormalize","Two-step norm: |xyz| then hypot with w. Rounds differently to a 4-term sqrt."},
        {"407d40","quatToMatrix","Quat -> 4x4 row-major, row-vector convention, s=2/|q|^2."},
        {"40c7f0","mat4Mul",""},
        {"40cbb0","mat4GetRow3","Translation row (+0x30)."},
        {"40cbd0","mat4GetRow1","Local Y axis (up) (+0x10)."},
        {"40cbf0","mat4GetRow2","Local Z axis (forward) (+0x20)."},
        {"406640","mat4GetRow0","Local X axis (right) (+0x00)."},
        {"40f690","vec3Sub",""},
        {"40f6c0","vec3Scale",""},
        {"40f6f0","vec3Dot",""},
        {"410130","vec3AngleDeg","acos(dot) * 180/pi."},
        // --- forces ---
        {"4f0910","carAccumForces","Builds the (point,force) list: gravity, drag, suspension, contacts."},
        {"4ed970","carDragForce","F = -vhat * (coeffAirResistance*|v|^2 + coeffFrictionBearings)."},
        {"4f0be0","carSuspTorque","Suspension torque decomposed on car X/Z, scaled by coeffMomentOX/OZ; Y discarded."},
        {"4f1000","carSuspForce","Suspension force from the normal-projected pass."},
        {"4f0d70","carSuspBuildForces","One (point,force) pair per loaded corner, along car local Y."},
        {"4f0f80","suspSpringDamper","spring=-((travel2+travel-rest)*kPos); damp=kSpeed*dtravel/dt; clamped."},
        {"4edac0","carContactSolve","Simultaneous contact + friction solve (Baraff contact-force method)."},
        {"4ee280","carGatherContacts",""},
        {"4ee5e0","carWheelForces",""},
        {"4eea50","carEngineForce",""},
        {"4eeea0","carTireForce",""},
        {"4ee130","carTireGripMode",""},
        {"4ee180","carTireGrip",""},
        {"4ef9e0","carGatherCollSpheres","Collision spheres as (x,y,z,radius) quads."},
        {"4efdc0","carCheckPenetration",""},
        {"4f01f0","carCheckContact",""},
        {"4f66e0","carPostSetState",""},
        {"4f5400","carStateDeriv",""},
        // --- per-car tuning getters (index = phys+0x54) ---
        {"4f73d0","cfgEngineParams","-> 0x014c46d0 + i*0x4c"},
        {"4f7400","cfgSteeringParams","-> 0x014c4868 + i*8 (angleSteer)"},
        {"4f7420","cfgTireParams","-> 0x014c4ae8 + i*0x34 (CoeffFront/RearTires, drift, sand, water)"},
        {"4f7450","cfgResistParams","-> 0x014c4b90 + i*8 (coeffFrictionBearings, coeffAirResistance)"},
        {"4f7470","cfgCdtParams","-> 0x014c4d00 + i*0xb4 (collision points)"},
        {"4f74a0","cfgSpringParams","-> 0x014c4c58 + i*0x38 (coeffMomentOX/OZ, length, lenMin/Max, kPos, kSpeed...)"},
    };

    @Override
    public void run() throws Exception {
        int named = 0, made = 0;
        for (String[] e : N) {
            Address a = toAddr(Long.parseLong(e[0], 16));
            Function f = getFunctionAt(a);
            if (f == null) {
                f = createFunction(a, null);
                if (f != null) made++;
            }
            if (f == null) { println("SKIP (no function) " + e[0]); continue; }
            f.setName(e[1], SourceType.USER_DEFINED);
            if (e[2].length() > 0) f.setComment(e[2]);
            named++;
        }
        println("named=" + named + " created=" + made);
    }
}
