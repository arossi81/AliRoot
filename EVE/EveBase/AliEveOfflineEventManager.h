//
//  AliEveOfflineEventManager.h
//  xAliRoot
//
//  Created by Jeremi Niedziela on 01/06/15.
//
//

#ifndef __AliEveOfflineEventManager__
#define __AliEveOfflineEventManager__

#include "AliEveEventManager.h"

class AliEveOfflineEventManager : public AliEveEventManager
{
public:
    AliEveOfflineEventManager(Int_t ev=0, bool storageManager=false);
    ~AliEveOfflineEventManager();
private:
    
   ClassDef(AliEveOfflineEventManager, 0);
};


#endif
