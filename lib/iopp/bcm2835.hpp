#include "arm/generate_mask_t.hpp"
#include "arm/reg_t.hpp"
#include "arm/ro_t.hpp"
#include "arm/rw_t.hpp"
#include "arm/wo_t.hpp"

namespace bcm2835 {
  struct io
  {
    static constexpr unsigned base_addr = 0x20000000;
  };
  struct power_management
  {
    static constexpr unsigned addr = io::base_addr + 0x100000;
    static constexpr unsigned password = 0x5a000000;
    static constexpr unsigned full_reset = 0x00000020;

    /*#define PM_RSTS_HADPOR_SET                                 0x00001000
    #define PM_RSTS_HADSRH_SET                                 0x00000400
    #define PM_RSTS_HADSRF_SET                                 0x00000200
    #define PM_RSTS_HADSRQ_SET                                 0x00000100
    #define PM_RSTS_HADWRH_SET                                 0x00000040
    #define PM_RSTS_HADWRF_SET                                 0x00000020
    #define PM_RSTS_HADWRQ_SET                                 0x00000010 // reboot=q. on reboot, don't clear flag
    #define PM_RSTS_HADDRH_SET                                 0x00000004 // halt
    #define PM_RSTS_HADDRF_SET                                 0x00000002 //
    #define PM_RSTS_HADDRQ_SET                                 0x00000001 //
    */
    using reset = reg_t<rw_t, addr + 0x1C, 0, 32>;
    using watchdog = reg_t<rw_t, addr + 0x24, 0, 32>;
  };
}
