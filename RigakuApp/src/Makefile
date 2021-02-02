TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

LIBRARY_IOC = ADRigaku
LIB_SRCS += ADRigaku.cpp

# Adjust directory if the acqmanager library is installed somewhere else 
acqmanager_DIR += $(RIGAKU_LIB)
ADRigaku_LIBS += acqmanager

DBD += RigakuSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

