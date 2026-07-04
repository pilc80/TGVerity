#include "tgverity/state_machine.h"

namespace tgverity {

const char* statusName(MessageStatus s) {
    switch (s) {
    case MessageStatus::local_pending:   return "local_pending";
    case MessageStatus::tg_sent:         return "tg_sent";
    case MessageStatus::tgverity_ack:    return "tgverity_ack";
    case MessageStatus::cleanup_pending: return "cleanup_pending";
    case MessageStatus::cleanup_done:    return "cleanup_done";
    case MessageStatus::failed:          return "failed";
    }
    return "?";
}

bool StateMachine::canTransition(MessageStatus from, MessageStatus to) {
    if (to == MessageStatus::failed) {
        return from != MessageStatus::cleanup_done;
    }
    switch (from) {
    case MessageStatus::local_pending:   return to == MessageStatus::tg_sent;
    case MessageStatus::tg_sent:         return to == MessageStatus::tgverity_ack;
    case MessageStatus::tgverity_ack:    return to == MessageStatus::cleanup_pending;
    case MessageStatus::cleanup_pending: return to == MessageStatus::cleanup_done;
    case MessageStatus::cleanup_done:    return false;
    case MessageStatus::failed:          return false;
    }
    return false;
}

bool StateMachine::transition(MessageStatus& current, MessageStatus next) {
    if (!canTransition(current, next)) {
        return false;
    }
    current = next;
    return true;
}

} // namespace tgverity
