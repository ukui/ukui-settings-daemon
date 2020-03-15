#include "xrdb.h"
#include "ukui-xrdb-manager.h"

Xrdb::Xrdb()
{
    m_pIXdbMgr = new ukuiXrdbManager();
}

Xrdb::~Xrdb() {
    if (m_pIXdbMgr) {
        delete m_pIXdbMgr;
    }
    m_pIXdbMgr = nullptr;
}

void Xrdb::activate () {
    m_pIXdbMgr->start();
}

void Xrdb::deactivate () {
    m_pIXdbMgr->stop();
}
