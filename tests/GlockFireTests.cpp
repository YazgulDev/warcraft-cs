#include "../src/combat/BurstFire.hpp"
#include <cassert>

int main() {
    // The first round is already debited; only the two remaining physical rounds are scheduled.
    BurstFire burst;assert(burst.Ready(100));burst.Begin(100,2,100,500);
    assert(!burst.Ready(100) && !burst.Due(199) && burst.Due(200));
    burst.Fired(200);assert(!burst.Due(299) && burst.Due(300));burst.Fired(300);
    assert(!burst.Active() && !burst.Due(400) && !burst.Ready(599) && burst.Ready(600));
    // One or two rounds in the magazine cannot turn into a free third round.
    burst.Begin(600,0,100,500);assert(!burst.Active() && !burst.Ready(700));
    burst.Begin(1200,1,100,500);assert(burst.Due(1300));burst.Fired(1300);assert(!burst.Active());
    // Menus, weapon switches, reload, loss of attack control and focus use the same explicit cancellation.
    burst.Begin(2000,2,100,500);burst.Cancel();assert(!burst.Due(3000) && burst.Ready(3000));
    // Late frames never emit overdue rounds together, and native millisecond clock wrap remains safe.
    burst.Begin(3000,2,100,500);assert(burst.Due(3400));burst.Fired(3400);
    assert(!burst.Due(3400) && burst.Due(3500));burst.Fired(3500);assert(burst.Ready(3500));
    burst.Begin(0xfffffff0u,2,100,500);assert(!burst.Due(20) && burst.Due(84));
    burst.Fired(84);burst.Fired(184);assert(!burst.Ready(400) && burst.Ready(484));
}
