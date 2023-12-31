﻿/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    Probe.cs

Abstract:

    Z3 Managed API: Probes

Author:

    Christoph Wintersteiger (cwinter) 2012-03-21

Notes:
    
--*/

using System.Diagnostics;
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Z3
{
    /// <summary>  
    /// Probes are used to inspect a goal (aka problem) and collect information that may be used to decide
    /// which solver and/or preprocessing step will be used.
    /// The complete list of probes may be obtained using the procedures <c>Context.NumProbes</c>
    /// and <c>Context.ProbeNames</c>.
    /// It may also be obtained using the command <c>(help-tactic)</c> in the SMT 2.0 front-end.
    /// </summary>
    public class Probe : Z3Object
    {
        /// <summary>
        /// Execute the probe over the goal. 
        /// </summary>
        /// <returns>A probe always produce a double value.
        /// "Boolean" probes return 0.0 for false, and a value different from 0.0 for true.</returns>
        public double Apply(Goal g)
        {
            Debug.Assert(g != null);

            Context.CheckContextMatch(g);
            return Native.Z3_probe_apply(Context.nCtx, NativeObject, g.NativeObject);
        }

        /// <summary>
        /// Apply the probe to a goal.
        /// </summary>
        public double this[Goal g]
        {
            get
            {
                Debug.Assert(g != null);

                return Apply(g);
            }
        }

        #region Internal
        internal Probe(Context ctx, IntPtr obj)
            : base(ctx, obj)
        {
            Debug.Assert(ctx != null);
        }
        internal Probe(Context ctx, string name)
            : base(ctx, Native.Z3_mk_probe(ctx.nCtx, name))
        {
            Debug.Assert(ctx != null);
        }

        internal override void IncRef(IntPtr o)
        {
            Native.Z3_probe_inc_ref(Context.nCtx, o);
        }

        internal override void DecRef(IntPtr o)
        {
            lock (Context)
            {
                if (Context.nCtx != IntPtr.Zero)
                    Native.Z3_probe_dec_ref(Context.nCtx, o);
            }
        }
        #endregion
    }
}
