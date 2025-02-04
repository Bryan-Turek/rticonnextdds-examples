/*
 * (c) Copyright, Real-Time Innovations, 2020.  All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the software solely for use with RTI Connext DDS. Licensee may
 * redistribute copies of the software provided that all such copies are subject
 * to this license. The software is provided "as is", with no warranty of any
 * type, including any warranty for fitness for any purpose. RTI is under no
 * obligation to maintain or support the software. RTI shall not be liable for
 * any incidental or consequential damages arising out of the use or inability
 * to use the software.
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "real_time_wan_transport.h"
#include "real_time_wan_transportSupport.h"
#include "ndds/ndds_cpp.h"
#include "ndds/ndds_namespace_cpp.h"
#include "application.h"
#include "INIReader.h"

using namespace DDS;

using namespace application;

static int shutdown_participant(
        DomainParticipant *participant,
        const char *shutdown_message,
        int status);

int run_publisher_application(
        unsigned int domain_id,
        unsigned int sample_count,
        unsigned int scenario,
        char *config_file)
{
    const char *libraryName = "RWT_Library";
    char profileName[64];

    sprintf(profileName, "Publisher_Scenario_%d", scenario);

    std::cout << "Executing scenario " << scenario << std::endl;

    // Start communicating in a domain, usually one participant per application
    DomainParticipant *participant =
            TheParticipantFactory->create_participant_with_profile(
                    domain_id,
                    libraryName,
                    profileName,
                    NULL /* listener */,
                    STATUS_MASK_NONE);
    if (participant == NULL) {
        return shutdown_participant(
                participant,
                "create_participant error",
                EXIT_FAILURE);
    }

    // A Publisher allows an application to create one or more DataWriters
    Publisher *publisher = participant->create_publisher(
            PUBLISHER_QOS_DEFAULT,
            NULL /* listener */,
            STATUS_MASK_NONE);
    if (publisher == NULL) {
        return shutdown_participant(
                participant,
                "create_publisher error",
                EXIT_FAILURE);
    }

    // Register the datatype to use when creating the Topic
    const char *type_name = HelloWorldTypeSupport::get_type_name();
    ReturnCode_t retcode =
            HelloWorldTypeSupport::register_type(participant, type_name);
    if (retcode != RETCODE_OK) {
        return shutdown_participant(
                participant,
                "register_type error",
                EXIT_FAILURE);
    }

    // Create a Topic with a name and a datatype
    Topic *topic = participant->create_topic(
            "Example HelloWorld",
            type_name,
            TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            STATUS_MASK_NONE);
    if (topic == NULL) {
        return shutdown_participant(
                participant,
                "create_topic error",
                EXIT_FAILURE);
    }

    // This DataWriter writes data on "Example HelloWorld" Topic
    DataWriter *untyped_writer = publisher->create_datawriter_with_profile(
            topic,
            libraryName,
            profileName,
            NULL /* listener */,
            STATUS_MASK_NONE);
    if (untyped_writer == NULL) {
        return shutdown_participant(
                participant,
                "create_datawriter error",
                EXIT_FAILURE);
    }

    // Narrow casts from an untyped DataWriter to a writer of your type
    HelloWorldDataWriter *typed_writer =
            HelloWorldDataWriter::narrow(untyped_writer);
    if (typed_writer == NULL) {
        return shutdown_participant(
                participant,
                "DataWriter narrow error",
                EXIT_FAILURE);
    }

    // Create data for writing, allocating all members
    HelloWorld *data = HelloWorldTypeSupport::create_data();
    if (data == NULL) {
        return shutdown_participant(
                participant,
                "HelloWorldTypeSupport::create_data error",
                EXIT_FAILURE);
    }

    INIReader reader(config_file);

    if (reader.ParseError() < 0) {
        std::cout << "Can't load '" << config_file << "'\n";
        return shutdown_participant(
                participant,
                "HelloWorldTypeSupport::create_data config_file unreadable",
                EXIT_FAILURE);
    }

    unsigned int init_count = (unsigned int)reader.GetInteger("message", "number", 0);
    std::string config_message = reader.Get("message", "message", "Hello World");

    // Main loop, write data
    for (unsigned int samples_written = init_count;
         !shutdown_requested && samples_written < sample_count;
         ++samples_written) {
        // Modify the data to be written here
        sprintf(data->msg, "%s %u", config_message.c_str(), samples_written);

        std::cout << "Writing " << config_message << ", count " << samples_written
                  << std::endl;
        retcode = typed_writer->write(*data, HANDLE_NIL);
        if (retcode != RETCODE_OK) {
            std::cerr << "write error " << retcode << std::endl;
        }

        // Send once every second
        Duration_t send_period = { 1, 0 };
        NDDSUtility::sleep(send_period);
    }

    // Delete previously allocated HelloWorld, including all contained elements
    retcode = HelloWorldTypeSupport::delete_data(data);
    if (retcode != RETCODE_OK) {
        std::cerr << "HelloWorldTypeSupport::delete_data error " << retcode
                  << std::endl;
    }

    // Delete all entities (DataWriter, Topic, Publisher, DomainParticipant)
    return shutdown_participant(participant, "Shutting down", EXIT_SUCCESS);
}

// Delete all entities
static int shutdown_participant(
        DomainParticipant *participant,
        const char *shutdown_message,
        int status)
{
    ReturnCode_t retcode;

    std::cout << shutdown_message << std::endl;

    if (participant != NULL) {
        // Cleanup everything created by this Participant
        retcode = participant->delete_contained_entities();
        if (retcode != RETCODE_OK) {
            std::cerr << "delete_contained_entities error " << retcode
                      << std::endl;
            status = EXIT_FAILURE;
        }

        retcode = TheParticipantFactory->delete_participant(participant);
        if (retcode != RETCODE_OK) {
            std::cerr << "delete_participant error " << retcode << std::endl;
            status = EXIT_FAILURE;
        }
    }

    return status;
}

int main(int argc, char *argv[])
{
    // Parse arguments and handle control-C
    ApplicationArguments arguments;
    parse_arguments(arguments, argc, argv);
    if (arguments.parse_result == PARSE_RETURN_EXIT) {
        return EXIT_SUCCESS;
    } else if (arguments.parse_result == PARSE_RETURN_FAILURE) {
        return EXIT_FAILURE;
    }
    setup_signal_handlers();

    // Sets Connext verbosity to help debugging
    NDDSConfigLogger::get_instance()->set_verbosity(arguments.verbosity);

    int status = run_publisher_application(
            arguments.domain_id,
            arguments.sample_count,
            arguments.scenario,
            arguments.config_file);

    // Releases the memory used by the participant factory.  Optional at
    // application exit
    ReturnCode_t retcode = DomainParticipantFactory::finalize_instance();
    if (retcode != RETCODE_OK) {
        std::cerr << "finalize_instance error " << retcode << std::endl;
        status = EXIT_FAILURE;
    }

    return status;
}
