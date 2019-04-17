/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef OMR_Z_SYSTEMLINKAGEZOS_INCL
#define OMR_Z_SYSTEMLINKAGEZOS_INCL

#include <stdint.h>
#include "codegen/Linkage.hpp"
#include "codegen/LinkageConventionsEnum.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/SystemLinkage.hpp"
#include "codegen/snippet/PPA1Snippet.hpp"
#include "codegen/snippet/PPA2Snippet.hpp"
#include "cs2/arrayof.h"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/DataTypes.hpp"
#include "il/SymbolReference.hpp"

class TR_EntryPoint;
class TR_zOSGlobalCompilationInfo;
namespace TR { class S390ConstantDataSnippet; }
namespace TR { class S390JNICallDataSnippet; }
namespace TR { class AutomaticSymbol; }
namespace TR { class CodeGenerator; }
namespace TR { class Instruction; }
namespace TR { class LabelSymbol; }
namespace TR { class Node; }
namespace TR { class ParameterSymbol; }
namespace TR { class RegisterDependencyConditions; }
namespace TR { class ResolvedMethodSymbol; }
namespace TR { class Symbol; }
template <class T> class List;

enum TR_XPLinkFrameType
   {
   TR_XPLinkUnknownFrame,
   TR_XPLinkSmallFrame,
   TR_XPLinkIntermediateFrame,
   TR_XPLinkStackCheckFrame
   };

/**
 * XPLink call types whose value is encoded in NOP following call.
 * These values are encoded - so don't change
 */
enum TR_XPLinkCallTypes {
   TR_XPLinkCallType_BASR      =0,     ///< generic BASR
   TR_XPLinkCallType_BRASL7    =3,     ///< BRASL r7,ep
   TR_XPLinkCallType_BASR33    =7      ///< BASR  r3,r3
   };

namespace TR {

class S390zOSSystemLinkage : public TR::SystemLinkage
   {
   public:

   /** \brief
    *     Size (in bytes) of the XPLINK stack frame bias as defined by the system linkage.
    */
   static const size_t XPLINK_STACK_FRAME_BIAS = 2048;

   public:

   S390zOSSystemLinkage(TR::CodeGenerator* cg);

   virtual void generateInstructionsForCall(TR::Node * callNode, TR::RegisterDependencyConditions * dependencies, intptrj_t targetAddress, TR::Register * methodAddressReg, TR::Register * javaLitOffsetReg, TR::LabelSymbol * returnFromJNICallLabel, TR::S390JNICallDataSnippet * jniCallDataSnippet, bool isJNIGCPoint = true);

   virtual TR::Register* callNativeFunction(TR::Node * callNode, TR::RegisterDependencyConditions * dependencies, intptrj_t targetAddress, TR::Register * methodAddressReg, TR::Register * javaLitOffsetReg, TR::LabelSymbol * returnFromJNICallLabel, TR::S390JNICallDataSnippet * jniCallDataSnippet, bool isJNIGCPoint = true);

   virtual TR::RealRegister::RegNum getENVPointerRegister() { return TR::RealRegister::GPR5; }
   virtual TR::RealRegister::RegNum getCAAPointerRegister() { return TR::RealRegister::GPR12; }
   
   virtual int32_t getRegisterSaveOffset(TR::RealRegister::RegNum);
   virtual int32_t getOutgoingParameterBlockSize();

   uint32_t calculateInterfaceMappingFlags(TR::ResolvedMethodSymbol *method);

   static uint32_t shiftFloatParmDescriptorFlag(uint32_t fieldVal, int32_t floatParmNum)  { return (fieldVal) << (6*(3-floatParmNum)); } // accepts floatParmNum values 0,1,2,3
   bool updateFloatParmDescriptorFlags(uint32_t *parmDescriptorFields, TR::Symbol *funcSymbol, int32_t parmCount, int32_t argSize, TR::DataType dataType, int32_t *floatParmNum, uint32_t *lastFloatParmAreaOffset, uint32_t *parmAreaOffset);

   static uint32_t getFloatParmDescriptorFlag(uint32_t descriptorFields, int32_t floatParmNum)  { return  (descriptorFields >> (6*(3-floatParmNum))) & 0x3F; }
   uint32_t calculateReturnValueAdjustFlag(TR::DataType dataType, int32_t aggregateLength);
   static uint32_t isFloatDescriptorFlagUnprototyped(uint32_t flag)  { return flag == 0; }

   uint32_t calculateCallDescriptorFlags(TR::Node *callNode);

   public:

   // == Related to signature of method or call
   uint32_t  getInterfaceMappingFlags() { return _interfaceMappingFlags; }
   void setInterfaceMappingFlags(uint32_t flags) { _interfaceMappingFlags = flags; }
   
   virtual void calculatePrologueInfo(TR::Instruction * cursor);
   virtual TR::Instruction *buyFrame(TR::Instruction * cursor, TR::Node *node);
   virtual void createEntryPointMarker(TR::Instruction *cursor, TR::Node *node);

   // === Call or entry related
   TR_XPLinkCallTypes genWCodeCallBranchCode(TR::Node *callNode, TR::RegisterDependencyConditions * deps);
   TR::Instruction * genCallNOPAndDescriptor(TR::Instruction * cursor, TR::Node *node, TR::Node *callNode, TR_XPLinkCallTypes callType);

   /** \brief
    *     Gets the label instruction which marks the stack pointer update instruction following it.
    *
    *  \return
    *     The stack pointer update label instruction if it exists; \c NULL otherwise.
    */
   TR::Instruction* getStackPointerUpdate() const;
   
   /** \brief
    *     Gets the PPA1 (Program Prologue Area 1) snippet for this method body.
    */
   TR::PPA1Snippet* getPPA1Snippet() const;
   
   /** \brief
    *     Gets the PPA2 (Program Prologue Area 2) snippet for this method body.
    */
   TR::PPA2Snippet* getPPA2Snippet() const;

   virtual void createPrologue(TR::Instruction * cursor);
   virtual void createEpilogue(TR::Instruction * cursor);

   private:

   TR::Instruction* getputFPRs(TR::InstOpCode::Mnemonic opcode, TR::Instruction *cursor, TR::Node *node, TR::RealRegister *spReg = NULL);

   virtual TR::Instruction* addImmediateToRealRegister(TR::RealRegister * targetReg, int32_t immediate, TR::RealRegister *tempReg, TR::Node *node, TR::Instruction *cursor, bool *checkTempNeeded=NULL);

   private:
   
   // TODO: There seems to be a lot of similarity between this relocation and PPA1OffsetToPPA2Relocation relocation.
   // It would be best if we common these up, perhaps adding an "offset" to to one of the existing relocation kinds
   // which perform similar relocations. See Relocation.hpp for which relocations may fit this criteria.
   class EntryPointMarkerOffsetToPPA1Relocation : public TR::LabelRelocation
      {
      public:

      EntryPointMarkerOffsetToPPA1Relocation(TR::Instruction* cursor, TR::LabelSymbol* ppa2);

      virtual void apply(TR::CodeGenerator* cg);

      private:

      TR::Instruction* _cursor;
      };

   TR::Instruction* _stackPointerUpdate;

   TR::PPA1Snippet* _ppa1;
   TR::PPA2Snippet* _ppa2;

   uint32_t _interfaceMappingFlags;       // describing the method body
   };
}

#endif
