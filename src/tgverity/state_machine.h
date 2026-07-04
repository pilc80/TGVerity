#pragma once

namespace tgverity {

// Virtual message lifecycle (see docs/message-lifecycle.md "Status model").
enum class MessageStatus {
    local_pending,
    tg_sent,
    tgverity_ack,
    cleanup_pending,
    cleanup_done,
    failed,
};

[[nodiscard]] const char* statusName(MessageStatus status);

class StateMachine {
public:
    [[nodiscard]] static bool canTransition(MessageStatus from, MessageStatus to);
    // Applies the transition if legal. Returns false (and leaves current unchanged) otherwise.
    static bool transition(MessageStatus& current, MessageStatus next);
};

} // namespace tgverity
