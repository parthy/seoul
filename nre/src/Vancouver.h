/*
 * Copyright (C) 2014, Markus Partheymueller <mpartheym@os.inf.tu-dresden.de>
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2009, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>
#include <services/VMManager.h>
#include <services/Console.h>
#include <services/Network.h>
#include <services/PCIConfig.h>
#include <kobj/Gsi.h>
#include <util/PCI.h>

#include <nul/motherboard.h>

#include "Timeouts.h"
#include "StorageDevice.h"
#include "VCPUBackend.h"
#include "ConsoleBackend.h"

extern nre::UserSm globalsm;
extern bool _dpci;

class Vancouver : public StaticReceiver<Vancouver> {
    static const uint64_t BASE_MAC  = 0x525402000000;

public:
    explicit Vancouver(const char **args, size_t count, size_t console, const nre::String &constitle,
                       size_t fbsize)
        : _clock(nre::Hip::get().freq_tsc * 1000), _mb(&_clock, nullptr), _timeouts(_mb),
          _conssess("console", console, constitle), _console(this, fbsize), _netsess(),
          _vmmng(), _vcpus(), _stdevs(), _pcicfg(), _acpi(), _pci() {
        try {
          _pcicfg = new nre::PCIConfigSession("pcicfg");
          _acpi = new nre::ACPISession("acpi");
          _pci = new nre::PCI(*_pcicfg, _acpi);
        } catch(...) {}

        // vmmanager is optional
        try {
            _vmmng = new nre::VMManagerSession("vmmanager");
            nre::Reference<nre::GlobalThread> vmmng = nre::GlobalThread::create(
                vmmng_thread, nre::CPU::current().log_id(), "vmm-vmmng");
            vmmng->set_tls<Vancouver*>(nre::Thread::TLS_PARAM, this);
            vmmng->start();
        }
        catch(const nre::Exception &e) {
            nre::Serial::get() << "Unable to connect to vmmanager: " << e.msg() << "\n";
        }

        // network is optional
        try {
            // TODO let the user customize the netsess
            _netsess = new nre::NetworkSession("network", 0);

            nre::Reference<nre::GlobalThread> network = nre::GlobalThread::create(
                network_thread, nre::CPU::current().log_id(), "vmm-network");
            network->set_tls<Vancouver*>(nre::Thread::TLS_PARAM, this);
            network->start();
        }
        catch(const nre::Exception &e) {
            nre::Serial::get() << "Unable to connect to network: " << e.msg() << "\n";
        }

        create_devices(args, count);
        create_vcpus();

        nre::Reference<nre::GlobalThread> input = nre::GlobalThread::create(
            keyboard_thread, nre::CPU::current().log_id(), "vmm-input");
        input->set_tls<Vancouver*>(nre::Thread::TLS_PARAM, this);
        input->start();
    }

    nre::ConsoleSession &console() {
        return _conssess;
    }
    Timeouts &timeouts() {
        return _timeouts;
    }
    uint64_t generate_mac() {
        static int macs = 0;
        if(_vmmng)
            return _vmmng->generate_mac().raw();
        return BASE_MAC + macs++;
    }

    void reset();
    bool receive(CpuMessage &msg);
    bool receive(MessageHostOp &msg);
    bool receive(MessagePciConfig &msg);
    bool receive(MessageAcpi &msg);
    bool receive(MessageTimer &msg);
    bool receive(MessageTime &msg);
    bool receive(MessageLegacy &msg);
    bool receive(MessageConsole &msg);
    bool receive(MessageDisk &msg);
    bool receive(MessageNetwork &msg);

private:
    static void network_thread(void*);
    static void keyboard_thread(void*);
    static void do_gsi(void*);
    static void vmmng_thread(void*);
    void create_devices(const char **args, size_t count);
    void create_vcpus();

    Clock _clock;
    Motherboard _mb;
    Timeouts _timeouts;
    nre::ConsoleSession _conssess;
    ConsoleBackend _console;
    nre::NetworkSession *_netsess;
    nre::VMManagerSession *_vmmng;
    nre::SList<VCPUBackend> _vcpus;
    StorageDevice *_stdevs[nre::Storage::MAX_CONTROLLER * nre::Storage::MAX_DRIVES];
    nre::PCIConfigSession *_pcicfg;
    nre::ACPISession *_acpi;
    nre::PCI *_pci;
    nre::DataSpace *_regs_ds;
};
