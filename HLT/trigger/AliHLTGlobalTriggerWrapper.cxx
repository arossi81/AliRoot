// $Id:  $
/**************************************************************************
 * This file is property of and copyright by the ALICE HLT Project        *
 * ALICE Experiment at CERN, All rights reserved.                         *
 *                                                                        *
 * Primary Authors: Artur Szostak <artursz@iafrica.com>                   *
 *                  for The ALICE HLT Project.                            *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/// @file   AliHLTGlobalTriggerWrapper.cxx
/// @author Artur Szostak <artursz@iafrica.com>
/// @date   28 Oct 2009
/// @brief  Implementation of the AliHLTGlobalTriggerWrapper interface class.
///
/// The AliHLTGlobalTriggerWrapper class is used to interface with an interpreted
/// class deriving from AliHLTGlobalTrigger. This is used when the global trigger
/// component is using CINT to interpret the trigger logic. The wrapper is necessary
/// to be able to call interpreted code from compiled code.

#include "AliHLTGlobalTriggerWrapper.h"
#include "TArrayL64.h"
#include "TClass.h"
#include "TInterpreter.h"

#ifdef R__BUILDING_CINT7
#include "cint7/Api.h"
#else
#include "Api.h"
#endif

ClassImp(AliHLTGlobalTriggerWrapper)


AliHLTGlobalTriggerWrapper::AliHLTGlobalTriggerWrapper(const char* classname) :
  AliHLTGlobalTrigger(),
  AliHLTLogging(),
  fClass(NULL),
  fObject(NULL),
  fFillFromMenuCall(),
  fNewEventCall(),
  fAddCall(),
  fCalculateTriggerDecisionCall(),
  fGetCountersCall(),
  fSetCountersCall()
{
  // The default constructor.
  
  fClass = TClass::GetClass(classname);
  if (fClass == NULL)
  {
    HLTError("Could not find class information for '%s'.", classname);
    return;
  }
  fFillFromMenuCall.InitWithPrototype(fClass, "FillFromMenu", "const AliHLTTriggerMenu&");
  if (not fFillFromMenuCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method FillFromMenu.", classname);
    return;
  }
  fNewEventCall.InitWithPrototype(fClass, "NewEvent", " ");  // Must have single whitespace in last parameter or we get a segfault in G__interpret_func.
  if (not fNewEventCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method NewEvent.", classname);
    return;
  }
  fAddCall.InitWithPrototype(fClass, "Add", "const TObject*, const AliHLTComponentDataType&, AliHLTUInt32_t");
  if (not fAddCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method Add.", classname);
    return;
  }
  fCalculateTriggerDecisionCall.InitWithPrototype(fClass, "CalculateTriggerDecision", "AliHLTTriggerDomain&, TString&");
  if (not fCalculateTriggerDecisionCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method CalculateTriggerDecision.", classname);
    return;
  }
  fGetCountersCall.InitWithPrototype(fClass, "GetCounters", " ");  // Must have single whitespace in last parameter or we get a segfault in G__interpret_func.
  if (not fGetCountersCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method GetCounters.", classname);
    return;
  }
  fSetCountersCall.InitWithPrototype(fClass, "SetCounters", "TArrayL64&");
  if (not fSetCountersCall.IsValid())
  {
    HLTError("Could not initialise method call object for class '%s' and method SetCounters.", classname);
    return;
  }
  fObject = fClass->New();
  if (fObject == NULL)
  {
    HLTError("Could not create a new object of type '%s'.", classname);
  }
}


AliHLTGlobalTriggerWrapper::~AliHLTGlobalTriggerWrapper()
{
  // Default destructor.
  
  fClass->Destructor(fObject);
}


void AliHLTGlobalTriggerWrapper::FillFromMenu(const AliHLTTriggerMenu& menu)
{
  // Fills internal values from the trigger menu.
  
  struct Params
  {
    const void* fMenu;
  } params;
  params.fMenu = &menu;
  fFillFromMenuCall.SetParamPtrs(&params, 1);
  fFillFromMenuCall.Execute(fObject);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
}


void AliHLTGlobalTriggerWrapper::NewEvent()
{
  // Clears the internal buffers for a new event.

  fNewEventCall.Execute(fObject);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
}


void AliHLTGlobalTriggerWrapper::Add(
      const TObject* object, const AliHLTComponentDataType& type,
      AliHLTUInt32_t spec
    )
{
  // Adds parameters from the object to the internal buffers and variables.
  
  struct Params
  {
    const void* fObj;
    const void* fType;
    long fSpec;
  } params;
  params.fObj = object;
  params.fType = &type;
  params.fSpec = spec;
  fAddCall.SetParamPtrs(&params, 3);
  fAddCall.Execute(fObject);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
}


bool AliHLTGlobalTriggerWrapper::CalculateTriggerDecision(AliHLTTriggerDomain& domain, TString& description)
{
  // Calculates the global trigger decision.

  struct Params
  {
    const void* fDomain;
    const void* fDesc;
  } params;
  params.fDomain = &domain;
  params.fDesc = &description;
  fCalculateTriggerDecisionCall.SetParamPtrs(&params, 2);
  Long_t retval;
  fCalculateTriggerDecisionCall.Execute(fObject, retval);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
  return bool(retval);
}


const TArrayL64& AliHLTGlobalTriggerWrapper::GetCounters() const
{
  // Returns the internal counter array.

  Long_t retval = 0x0;
  fGetCountersCall.Execute(fObject, retval);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
  static const TArrayL64 emptyArray;
  const TArrayL64* ptr = &emptyArray; // Make sure we do not return a NULL pointer.
  if (retval != 0x0) ptr = reinterpret_cast<const TArrayL64*>(retval);
  return *ptr;
}


void AliHLTGlobalTriggerWrapper::SetCounters(const TArrayL64& counters)
{
  // Fills the internal counter array with new values.
  
  struct Params
  {
    const void* fCounters;
  } params;
  params.fCounters = &counters;
  fSetCountersCall.SetParamPtrs(&params, 1);
  fSetCountersCall.Execute(fObject);
  int error = G__lasterror();
  if (error != TInterpreter::kNoError)
  {
    if (error == TInterpreter::kFatal)
    {
      HLTFatal("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
    else
    {
      HLTError("Error interpreting the code for class '%s' at line %d.", fClass->GetName(), G__lasterror_linenum());
    }
  }
}


bool AliHLTGlobalTriggerWrapper::IsValid() const
{
  // Checks if the wrapper class was initialised correctly.
  
  return fClass != NULL and fObject != NULL and fFillFromMenuCall.IsValid()
    and fNewEventCall.IsValid() and fAddCall.IsValid()
    and fCalculateTriggerDecisionCall.IsValid();
}

