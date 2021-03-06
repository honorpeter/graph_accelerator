#include "MemWrapper.h"
#include "pe.h"

int sc_main(int argc, char *argv[]){

    sc_set_time_resolution(1, SC_NS);

    // Only burstIdx is transferred.
    sc_signal<long> burstReq[PNUM];
    sc_signal<long> burstResp[PNUM];
    sc_signal<bool> bfsDone;

    double peClkCycle = 2500;
    double memClkCycle = 625;
    sc_clock peClk("peClk", peClkCycle, SC_NS, 0.5);

    GL::cfgBfsParam("./config.txt");
    MemWrapper memWrapper("memWrapper", memClkCycle, peClkCycle, argc, argv);
    memWrapper.setNewStartVertex(GL::startingVertices[0]);
    std::cout << "start vertex: " << GL::startingVertices[0] << std::endl;
    for(int i = 0; i < PNUM; i++){
        memWrapper.burstReq[i](burstReq[i]);
        memWrapper.burstResp[i](burstResp[i]);
    }
    memWrapper.bfsDone(bfsDone);
    memWrapper.sigInit();

    pe peInst("peInst", 0, peClkCycle);
    for(int i = 0; i < PNUM; i++){
        peInst.burstReq[i](burstReq[i]);
        peInst.burstResp[i](burstResp[i]);
    }
    peInst.bfsDone(bfsDone);
    peInst.peClk(peClk);
    peInst.sigInit();

    sc_start();

    return 0;

}
