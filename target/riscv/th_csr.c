/*
 * Xuantie-specific CSRs.
 *
 * Copyright (c) 2024 VRULL GmbH
 * Copyright (c) 2024 Alibaba Group. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "cpu_vendorid.h"

typedef struct {
    int csrno;
    int (*insertion_test)(RISCVCPU *cpu);
    riscv_csr_operations csr_ops;
} riscv_csr;

static int test_thead_mvendorid(RISCVCPU *cpu)
{
    if (cpu->cfg.mvendorid != THEAD_VENDOR_ID) {
        return -1;
    }
    return 0;
}

#if !defined(CONFIG_USER_ONLY)
/*
 * CSR_TH_SXSTATUS = CSR_SXSTATUS
 * Not certain if CSR_TH_SXSTATUS can be suitable
 * Use the old way to add custom CSRs
 */
static int test_always_false(RISCVCPU *cpu)
{
    return -1;
}
#endif /* !CONFIG_USER_ONLY */

/*
 * In XTheadVector, vcsr is inaccessible
 * and we just check ext_xtheadvector instead of ext_zve32f
 */
static RISCVException th_vs(CPURISCVState *env, int csrno)
{
    RISCVCPU *cpu = env_archcpu(env);
    if (cpu->cfg.ext_xtheadvector) {
        if (csrno == CSR_VCSR) {
            return RISCV_EXCP_ILLEGAL_INST;
        }
        return RISCV_EXCP_NONE;
    }
    return vs(env, csrno);
}

static RISCVException
th_read_fcsr(CPURISCVState *env, int csrno, target_ulong *val)
{
    RISCVCPU *cpu = env_archcpu(env);
    RISCVException ret = read_fcsr(env, csrno, val);
    if (cpu->cfg.ext_xtheadvector) {
        *val = set_field(*val, TH_FSR_VXRM,  env->vxrm);
        *val = set_field(*val, TH_FSR_VXSAT,  env->vxsat);
    }
    return ret;
}

static RISCVException
th_write_fcsr(CPURISCVState *env, int csrno, target_ulong val)
{
    RISCVCPU *cpu = env_archcpu(env);
    if (cpu->cfg.ext_xtheadvector) {
        env->vxrm = get_field(val, TH_FSR_VXRM);
        env->vxsat = get_field(val, TH_FSR_VXSAT);
    }
    return write_fcsr(env, csrno, val);
}

/*
 * We use the RVV1.0 format for env->vtype
 * When reading vtype, we need to change the format.
 * In RVV1.0:
 *   vtype[7] -> vma
 *   vtype[6] -> vta
 *   vtype[5:3] -> vsew
 *   vtype[2:0] -> vlmul
 * In XTheadVector:
 *   vtype[6:5] -> vediv
 *   vtype[4:2] -> vsew
 *   vtype[1:0] -> vlmul
 * Although vlmul size is different between RVV1.0 and XTheadVector,
 * the lower 2 bits have the same meaning.
 * vma, vta and vediv are useless in XTheadVector, So we need to clear
 * vtype[7:5] for XTheadVector
 */
static RISCVException
th_read_vtype(CPURISCVState *env, int csrno, target_ulong *val)
{
    RISCVCPU *cpu = env_archcpu(env);
    RISCVException ret = read_vtype(env, csrno, val);
    if (cpu->cfg.ext_xtheadvector) {
        *val = set_field(*val, TH_VTYPE_LMUL,
                          FIELD_EX64(*val, VTYPE, VLMUL));
        *val = set_field(*val, TH_VTYPE_SEW,
                          FIELD_EX64(*val, VTYPE, VSEW));
        *val = set_field(*val, TH_VTYPE_CLEAR, 0);
    }
    return ret;
}

#if !defined(CONFIG_USER_ONLY)
static RISCVException
th_read_mstatus(CPURISCVState *env, int csrno, target_ulong *val)
{
    RISCVCPU *cpu = env_archcpu(env);
    RISCVException ret = read_mstatus(env, csrno, val);
    if (cpu->cfg.ext_xtheadvector) {
        *val = set_field(*val, TH_MSTATUS_VS,
                         get_field(*val, MSTATUS_VS));
    }
    return ret;
}

static RISCVException
th_write_mstatus(CPURISCVState *env, int csrno, target_ulong val)
{
    RISCVCPU *cpu = env_archcpu(env);
    if (cpu->cfg.ext_xtheadvector) {
        val = set_field(val, MSTATUS_VS,
                        get_field(val, TH_MSTATUS_VS));
    }
    return write_mstatus(env, csrno, val);
}

static RISCVException
th_read_sstatus(CPURISCVState *env, int csrno, target_ulong *val)
{
    RISCVCPU *cpu = env_archcpu(env);
    RISCVException ret = read_sstatus(env, csrno, val);
    if (cpu->cfg.ext_xtheadvector) {
        *val = set_field(*val, TH_MSTATUS_VS,
                        get_field(*val, MSTATUS_VS));
    }
    return ret;
}

static RISCVException
th_write_sstatus(CPURISCVState *env, int csrno, target_ulong val)
{
    RISCVCPU *cpu = env_archcpu(env);
    if (cpu->cfg.ext_xtheadvector) {
        val = set_field(val, MSTATUS_VS,
                        get_field(val, TH_MSTATUS_VS));
    }
    return write_sstatus(env, csrno, val);
}

static RISCVException s_mode_csr(CPURISCVState *env, int csrno)
{
    if (env->debugger)
        return RISCV_EXCP_NONE;

    if (env->priv >= PRV_S)
        return RISCV_EXCP_NONE;

    return RISCV_EXCP_ILLEGAL_INST;
}

static RISCVException read_th_sxstatus(CPURISCVState *env, int csrno,
                                       target_ulong *val)
{
    /* We don't set MAEE here, because QEMU does not implement MAEE. */
    *val = TH_SXSTATUS_UCME | TH_SXSTATUS_THEADISAEE; // |
           //riscv_cpu_cfg(env)->ext_xtheadmaee * TH_SXSTATUS_MAEE;
    return RISCV_EXCP_NONE;
}

static RISCVException read_fxcr(CPURISCVState *env, int csrno, target_ulong *val)
{
    *val = (env->bf16 << FXCR_BF16_SHIFT) |
           (riscv_cpu_get_fflags(env) << FSR_AEXC_SHIFT) |
           (env->frm << FXCR_RD_SHIFT);
    return RISCV_EXCP_NONE;
}
static int write_fxcr(CPURISCVState *env, int csrno, target_ulong val)
{
#if !defined(CONFIG_USER_ONLY)
    env->mstatus |= MSTATUS_FS;
#endif
    env->bf16 = (val & FXCR_BF16) >> FXCR_BF16_SHIFT;
    env->frm = (val & FXCR_RD) >> FXCR_RD_SHIFT;
    riscv_cpu_set_fflags(env, (val & FSR_AEXC) >> FSR_AEXC_SHIFT);
    return RISCV_EXCP_NONE;
}

static RISCVException read_ignore(CPURISCVState *env, int csrno, target_ulong *val)
{
    return RISCV_EXCP_NONE;
}

static int write_ignore(CPURISCVState *env, int csrno, target_ulong val)
{
    return RISCV_EXCP_NONE;
}
#endif

static riscv_csr th_csr_list[] = {
    {
        .csrno = CSR_FCSR,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "fcsr", fs, th_read_fcsr, th_write_fcsr }
    },
    {
        .csrno = CSR_VSTART,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vstart", th_vs, read_vstart, write_vstart }
    },
    {
        .csrno = CSR_VXSAT,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vxsat", th_vs, read_vxsat, write_vxsat }
    },
    {
        .csrno = CSR_VXRM,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vxrm", th_vs, read_vxrm, write_vxrm }
    },
    {
        .csrno = CSR_VCSR,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vcsr", th_vs, read_vcsr, write_vcsr }
    },
    {
        .csrno = CSR_VL,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vl", th_vs, read_vl}
    },
    {
        .csrno = CSR_VTYPE,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vtype", th_vs, th_read_vtype}
    },
    {
        .csrno = CSR_VLENB,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "vlenb", th_vs, read_vlenb}
    },
    {
        .csrno = CSR_FXCR,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "fxcr", fs, read_fxcr,    write_fxcr }
    },
    {
        .csrno = CSR_MHCR,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "mhcr", any, read_ignore,    write_ignore }
    },
    {
        .csrno = CSR_MCOR,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "mcor", any, read_ignore,    write_ignore }
    },
#if !defined(CONFIG_USER_ONLY)
    {
        .csrno = CSR_MSTATUS,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "mstatus", any, th_read_mstatus, th_write_mstatus}
    },
    {
        .csrno = CSR_SSTATUS,
        .insertion_test = test_thead_mvendorid,
        .csr_ops = { "sstatus", smode, th_read_sstatus, th_write_sstatus}
    },
    {
        .csrno = CSR_TH_SXSTATUS,
        .insertion_test = test_always_false,
        .csr_ops = { "th.sxstatus", s_mode_csr, read_th_sxstatus }
    }

#endif /* !CONFIG_USER_ONLY */
};

void th_register_custom_csrs(RISCVCPU *cpu)
{
    for (size_t i = 0; i < ARRAY_SIZE(th_csr_list); i++) {
        int csrno = th_csr_list[i].csrno;
        riscv_csr_operations *csr_ops = &th_csr_list[i].csr_ops;
        if (!th_csr_list[i].insertion_test(cpu))
            riscv_set_csr_ops(csrno, csr_ops);
    }
}
