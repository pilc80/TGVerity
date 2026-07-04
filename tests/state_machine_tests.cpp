#include "tgverity/state_machine.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace tgverity;

    auto s = MessageStatus::local_pending;
    assert(StateMachine::transition(s, MessageStatus::tg_sent));
    assert(s == MessageStatus::tg_sent);
    assert(StateMachine::transition(s, MessageStatus::tgverity_ack));
    assert(StateMachine::transition(s, MessageStatus::cleanup_pending));
    assert(StateMachine::transition(s, MessageStatus::cleanup_done));
    assert(s == MessageStatus::cleanup_done);

    // cleanup_done is terminal: no further transition, not even failed.
    assert(!StateMachine::transition(s, MessageStatus::tg_sent));
    assert(!StateMachine::transition(s, MessageStatus::failed));
    assert(s == MessageStatus::cleanup_done);

    // Illegal forward jump is rejected, state unchanged.
    auto t = MessageStatus::local_pending;
    assert(!StateMachine::transition(t, MessageStatus::tgverity_ack));
    assert(t == MessageStatus::local_pending);

    // Failure allowed from any non-terminal state; failed is terminal.
    auto u = MessageStatus::tg_sent;
    assert(StateMachine::transition(u, MessageStatus::failed));
    assert(u == MessageStatus::failed);
    assert(!StateMachine::transition(u, MessageStatus::tg_sent));

    std::cout << "state_machine_tests passed\n";
    return 0;
}
