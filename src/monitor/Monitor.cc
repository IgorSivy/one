/* -------------------------------------------------------------------------- */
/* Copyright 2002-2019, OpenNebula Project, OpenNebula Systems                */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#include "monitor.h"
#include "MonitorTemplate.h"
#include "NebulaLog.h"
#include "Client.h"

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

void Monitor::start()
{
    // System directories
    string log_file;
    string etc_path;

    const char *nl = getenv("ONE_LOCATION");

    if (nl == 0) // OpenNebula installed under root directory
    {
        log_file = "/var/log/one/monitor.log";
        etc_path = "/etc/one/";
    }
    else
    {
        log_file = string(nl) + "/var/monitor.log";
        etc_path = string(nl) + "/etc/";
    }

    // Configuration File
    MonitorTemplate conf(etc_path);
    if ( conf.load_configuration() != 0 )
    {
        throw runtime_error("Error reading configuration file.");
    }
    // todo load configuration values

    // Log system
    NebulaLog::LogType log_system = NebulaLog::STD;
    Log::MessageType clevel = Log::WARNING;

    const VectorAttribute *log = conf.get("LOG");

    if (log != 0)
    {
        string value;
        int ilevel;

        value      = log->vector_value("SYSTEM");
        log_system = NebulaLog::str_to_type(value);

        value  = log->vector_value("DEBUG_LEVEL");
        ilevel = atoi(value.c_str());

        if (Log::ERROR <= ilevel && ilevel <= Log::DDDEBUG)
        {
            clevel = static_cast<Log::MessageType>(ilevel);
        }
    }

    if (log_system != NebulaLog::UNDEFINED)
    {
        NebulaLog::init_log_system(log_system,
                                   clevel,
                                   log_file.c_str(),
                                   ios_base::trunc,
                                   "one_monitor");
    }
    else
    {
        throw runtime_error("Unknown LOG_SYSTEM.");
    }

    NebulaLog::log("MON", Log::INFO, "Init Monitor Log system");


    ostringstream oss;
    oss << "Starting Scheduler Daemon" << endl;
    oss << "----------------------------------------\n";
    oss << "     Scheduler Configuration File       \n";
    oss << "----------------------------------------\n";
    oss << conf;
    oss << "----------------------------------------";

    NebulaLog::log("MON", Log::INFO, oss);

    // -----------------------------------------------------------
    // XML-RPC Client
    // -----------------------------------------------------------
    // {
    //     string       one_xmlrpc;
    //     long long    message_size;
    //     unsigned int timeout;

    //     conf.get("ONE_XMLRPC", one_xmlrpc);
    //     conf.get("MESSAGE_SIZE", message_size);
    //     conf.get("TIMEOUT", timeout);

    //     Client::initialize("", one_xmlrpc, message_size, timeout);

    //     oss.str("");

    //     oss << "XML-RPC client using " << (Client::client())->get_message_size()
    //         << " bytes for response buffer.\n";

    //     NebulaLog::log("MON", Log::INFO, oss);
    // }

    // xmlInitParser();

    // -----------------------------------------------------------
    // Close stds, we no longer need them
    // -----------------------------------------------------------
    if (NebulaLog::log_type() != NebulaLog::STD )
    {
        int fd = open("/dev/null", O_RDWR);

        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);

        close(fd);

        fcntl(0, F_SETFD, 0); // Keep them open across exec funcs
        fcntl(1, F_SETFD, 0);
        fcntl(2, F_SETFD, 0);
    }
    else
    {
        fcntl(0, F_SETFD, FD_CLOEXEC);
        fcntl(1, F_SETFD, FD_CLOEXEC);
        fcntl(2, F_SETFD, FD_CLOEXEC);
    }

    // -----------------------------------------------------------
    // Block all signals before creating any Nebula thread
    // -----------------------------------------------------------
    sigset_t    mask;
    int         signal;

    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    one_util::SSLMutex::initialize();

    // -----------------------------------------------------------
    // Create the monitor loop
    // -----------------------------------------------------------

    // NebulaLog::log("MON", Log::INFO, "Starting monitor loop...");

    // pthread_attr_t pattr;
    // pthread_attr_init(&pattr);
    // pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

    // int rc = pthread_create(&monitor_thread, &pattr, scheduler_action_loop, (void *) this);

    // if (rc != 0)
    // {
    //     NebulaLog::log("MON", Log::ERROR,
    //         "Could not start monitor loop, exiting");

    //     return;
    // }

    // -----------------------------------------------------------
    // Wait for a SIGTERM or SIGINT signal
    // -----------------------------------------------------------

    sigemptyset(&mask);

    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    sigwait(&mask, &signal);

    //am.finalize();

    //pthread_join(sched_thread, 0);

    xmlCleanupParser();

    NebulaLog::finalize_log_system();
}
