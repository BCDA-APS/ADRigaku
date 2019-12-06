errlogInit(20000)

< envPaths
#epicsThreadSleep(20)
dbLoadDatabase("$(TOP)/dbd/RigakuApp.dbd")
RigakuApp_registerRecordDeviceDriver(pdbbase) 

RigakuConfig("TEST", 0, 0, 0, 0)
dbLoadRecords("$(TOP)/RigakuApp/Db/Rigaku.template", "P=8idRigaku:,R=cam1:,PORT=TEST,ADDR=0,TIMEOUT=1")

##########
iocInit()
##########

