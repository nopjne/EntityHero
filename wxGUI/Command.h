#pragma once
#include <vector>


class _CommandPattern {
public:
    virtual void Execute() = 0;
    virtual void Revert() = 0;
};

typedef std::shared_ptr<_CommandPattern> CommandPattern;

class _GroupedCommand : public _CommandPattern {
    std::vector<CommandPattern> m_CommandGroup;
public:
    virtual void Execute() {
        for (auto var : m_CommandGroup) {
            var->Execute();
        }
    }
    virtual void Revert() {
        for (auto var = m_CommandGroup.rbegin(); var != m_CommandGroup.rend(); var++) {
            var->get()->Revert();
        }
        // for (auto var : m_CommandGroup) {
        //     var->Revert();
        // }
    }

    virtual void PushCommand(CommandPattern Command) {
        m_CommandGroup.push_back(Command);
    }

    virtual bool Empty()
    {
        return m_CommandGroup.empty();
    }
};

typedef std::shared_ptr<_GroupedCommand> GroupedCommand;