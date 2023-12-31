#define REG_USI_CMGP0_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2000)
#define REG_I2C_CMGP0_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2004)
#define REG_USI_CMGP1_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2010)
#define REG_I2C_CMGP1_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2014)
#define REG_USI_CMGP2_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2020)
#define REG_I2C_CMGP2_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2024)
#define REG_USI_CMGP3_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2030)
#define REG_I2C_CMGP3_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2034)
#define REG_USI_CMGP4_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2040)
#define REG_I2C_CMGP4_SW_CONF                   (SYSREG_CMGP_BASE_ADDRESS + 0x2044)

#define SYSREG_CMGP_INTC_BASE                   (SYSREG_CMGP2CHUB_BASE_ADDRESS)//(CMGP_BASE + 0x70000)
#define REG_SYSREG_INTC_CONTROL                 (SYSREG_CMGP_INTC_BASE + 0x250)
#define REG_SYSREG_INTC0_IGROUP                 (SYSREG_CMGP_INTC_BASE + 0x254)
#define REG_SYSREG_INTC0_IEN_SET                (SYSREG_CMGP_INTC_BASE + 0x258)
#define REG_SYSREG_INTC0_IEN_CLR                (SYSREG_CMGP_INTC_BASE + 0x25C)
#define REG_SYSREG_INTC0_IPEND                  (SYSREG_CMGP_INTC_BASE + 0x260)
#define REG_SYSREG_INTC0_IPRIO_PEND             (SYSREG_CMGP_INTC_BASE + 0x264)
#define REG_SYSREG_INTC0_IPRIORITY0             (SYSREG_CMGP_INTC_BASE + 0x268)
#define REG_SYSREG_INTC0_IPRIORITY1             (SYSREG_CMGP_INTC_BASE + 0x26C)
#define REG_SYSREG_INTC0_IPRIORITY2             (SYSREG_CMGP_INTC_BASE + 0x270)
#define REG_SYSREG_INTC0_IPRIORITY3             (SYSREG_CMGP_INTC_BASE + 0x274)
#define REG_SYSREG_INTC0_IPRIO_SECURE_PEND      (SYSREG_CMGP_INTC_BASE + 0x278)
#define REG_SYSREG_INTC0_IPRIO_NONSECURE_PEND   (SYSREG_CMGP_INTC_BASE + 0x27C)

#define REG_SYSREG_INTC1_IGROUP                 (SYSREG_CMGP_INTC_BASE + 0x284)
#define REG_SYSREG_INTC1_IEN_SET                (SYSREG_CMGP_INTC_BASE + 0x288)
#define REG_SYSREG_INTC1_IEN_CLR                (SYSREG_CMGP_INTC_BASE + 0x28C)
#define REG_SYSREG_INTC1_IPEND                  (SYSREG_CMGP_INTC_BASE + 0x290)
#define REG_SYSREG_INTC1_IPRIO_PEND             (SYSREG_CMGP_INTC_BASE + 0x294)
#define REG_SYSREG_INTC1_IPRIORITY0             (SYSREG_CMGP_INTC_BASE + 0x298)
#define REG_SYSREG_INTC1_IPRIORITY1             (SYSREG_CMGP_INTC_BASE + 0x29C)
#define REG_SYSREG_INTC1_IPRIORITY2             (SYSREG_CMGP_INTC_BASE + 0x2A0)
#define REG_SYSREG_INTC1_IPRIORITY3             (SYSREG_CMGP_INTC_BASE + 0x2A4)
#define REG_SYSREG_INTC1_IPRIO_SECURE_PEND      (SYSREG_CMGP_INTC_BASE + 0x2A8)
#define REG_SYSREG_INTC1_IPRIO_NONSECURE_PEND   (SYSREG_CMGP_INTC_BASE + 0x2AC)
