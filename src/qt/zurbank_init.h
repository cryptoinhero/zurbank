#ifndef ZURBANK_QT_INIT_H
#define ZURBANK_QT_INIT_H

namespace ZurBank
{
    //! Shows an user dialog with general warnings and potential risks
    bool AskUserToAcknowledgeRisks();

    //! Setup and initialization related to ZURBank Qt
    bool Initialize();
}

#endif // ZURBANK_QT_INIT_H
