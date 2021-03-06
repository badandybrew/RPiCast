/*********************************************************************
 RPiCast ( Screencasting application using RaspberryPi )

 Copyright (C)  Anshul Routhu <anshul.m67@gmail.com>

 All rights reserved.

 This file unittests.cpp is part of RPiCast project

 RPiCast is a free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *********************************************************************/

#include "utils.h"
#include <UnitTest++/UnitTest++.h>
#include "basepipe.h"
#include "av_pipe.h"
#include "file_io.h"
#include "socket_io.h"

class TestDevice: public ADevice
{
public:
    TestDevice(std::string name) :
        ADevice(name)
    {
    }

    virtual ~TestDevice()
    {
    }

    virtual VC_STATUS Initialize()
    {
        m_inport = new InputPort("Inputport 0", this);
        m_outport = new OutputPort("Outport 0", this);
        return (VC_SUCCESS);
    }

    virtual VC_STATUS Uninitialize()
    {
        delete m_inport;
        delete m_outport;
        return (VC_SUCCESS);
    }

    virtual VC_STATUS Notify(VC_EVENT* evt)
    {
        return (VC_SUCCESS);
    }

    virtual InputPort* Input(int portno)
    {
        return (m_inport);
    }

    virtual OutputPort* Output(int portno)
    {
        return (m_outport);
    }

    virtual VC_STATUS SendCommand(VC_CMD cmd)
    {
        return (VC_SUCCESS);
    }

    virtual VC_STATUS SetParameters(const InputParams* params)
    {
        return (VC_SUCCESS);
    }

    virtual VC_STATUS GetParameters(OutputParams* params)
    {
        return (VC_SUCCESS);
    }

private:
    InputPort* m_inport;
    OutputPort* m_outport;

};

SUITE(APipeFrameworkTest)
{
TEST(PipeAPIs)
{
    FILE* f = fopen("FileSrc.in", "wb");
    fwrite("Hello World!", 1, 12, f);
    fclose(f);

    DBGPRINT(LEVEL_ALWAYS, ("Testing PipeAPIs\n"));
    BasePipe* pipe = new AVPipe("FileIO Pipe 0");

    pipe->AddDevice(VC_FILESINK_DEVICE, "PipeAPIFileSink", "FileSink.out");

    pipe->Initialize();
    OutputPort* output = new OutputPort("FileIO Output 0", NULL);
    InputPort* input = new InputPort("FileIO Input 0", NULL);
    Buffer* buf;

    CHECK_EQUAL(pipe->c_str(), "FileIO Pipe 0");
    CHECK_EQUAL(pipe->FindDevice(VC_FILESINK_DEVICE)->c_str(), "PipeAPIFileSink");
    CHECK_EQUAL(output->c_str(), "FileIO Output 0");
    CHECK_EQUAL(input->c_str(), "FileIO Input 0");
    CHECK_EQUAL(pipe->ConnectPorts(pipe->FindDevice(VC_FILESINK_DEVICE)->Input(0), output), VC_SUCCESS);

    CHECK_EQUAL(pipe->SendCommand(VC_CMD_START), VC_SUCCESS);
    usleep(100000); //wait for thread to start
    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->SetTag(TAG_START), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);

    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->WriteData((void* )"RPiCast", 7), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);

    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->SetTag(TAG_END), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);
    usleep(100000); // wait for fsink to process buffer
    CHECK_EQUAL(pipe->SendCommand(VC_CMD_STOP), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(pipe->FindDevice(VC_FILESINK_DEVICE)->Input(0), output), VC_SUCCESS);
    CHECK_EQUAL(pipe->Uninitialize(), VC_SUCCESS);
    CHECK_EQUAL(pipe->RemoveDevice(VC_FILESINK_DEVICE), VC_SUCCESS);

    CHECK_EQUAL(pipe->AddDevice(VC_FILESRC_DEVICE, "PipeAPIFileSrc", "FileSink.out"), VC_SUCCESS);
    CHECK_EQUAL(pipe->Initialize(), VC_SUCCESS);

    CHECK_EQUAL(pipe->FindDevice(VC_FILESRC_DEVICE)->c_str(), "PipeAPIFileSrc");
    CHECK_EQUAL(pipe->ConnectPorts(input, pipe->FindDevice(VC_FILESRC_DEVICE)->Output(0)), VC_SUCCESS);

    CHECK_EQUAL(pipe->SendCommand(VC_CMD_START), VC_SUCCESS);

    //wait untill a buffer is pushed but source device
    while (!input->IsBufferAvailable());
    CHECK(input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->GetTag(), TAG_START);
    CHECK_EQUAL(input->RecycleBuffer(buf), VC_SUCCESS);

    //wait untill a buffer is pushed by source device
    while (!input->IsBufferAvailable());
    CHECK(input->IsBufferAvailable());
    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "RPiCast", 7));
    CHECK_EQUAL(pipe->SendCommand(VC_CMD_STOP), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(input, pipe->FindDevice(VC_FILESRC_DEVICE)->Output(0)), VC_SUCCESS);
    pipe->Uninitialize();
    CHECK_EQUAL(pipe->RemoveDevice(VC_FILESRC_DEVICE), VC_SUCCESS);

    FILE* fp;
    char c[12];
    fp = fopen("FileSink.out", "rb");
    CHECK_EQUAL(fread(c, 1, 7, fp), (uint32_t )7);
    CHECK(!memcmp(c, "RPiCast", 7));

    fclose(fp);
    delete output;
    delete input;

    CHECK_EQUAL(pipe->AddDevice(VC_FILESRC_DEVICE, "PipeAPIFileSrc", "FileSrc.in"), VC_SUCCESS);
    CHECK_EQUAL(pipe->AddDevice(VC_FILESINK_DEVICE, "PipeAPIFileSink", "FileSink.out"), VC_SUCCESS);

    pipe->Initialize();
    pipe->Prepare();
    pipe->SendCommand(VC_CMD_START);

    usleep(10000);

    pipe->SendCommand(VC_CMD_STOP);
    pipe->Reset();
    pipe->Uninitialize();
    CHECK_EQUAL(pipe->RemoveDevice(VC_FILESRC_DEVICE), VC_SUCCESS);
    CHECK_EQUAL(pipe->RemoveDevice(VC_FILESINK_DEVICE), VC_SUCCESS);

    delete pipe;
}

TEST(ADeviceAPIs)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing ADeviceAPIs\n"));
    TestDevice* src = new TestDevice("TestDevice Src");
    CHECK_EQUAL(src->c_str(), "TestDevice Src");
    CHECK_EQUAL(src->Initialize(), VC_SUCCESS);
    CHECK_EQUAL(src->Notify(NULL), VC_SUCCESS);
    CHECK(!!src->Input(0));
    CHECK(!!src->Output(0));
    CHECK_EQUAL(src->SendCommand(VC_CMD_START), VC_SUCCESS);
    CHECK_EQUAL(src->SendCommand(VC_CMD_STOP), VC_SUCCESS);
    CHECK_EQUAL(src->Uninitialize(), VC_SUCCESS);

    delete src;
}

TEST(ADeviceConnections)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing ADeviceConnections\n"));
    BasePipe* pipe = new BasePipe("Pipe 0");
    ADevice* src = new TestDevice("TestDevice Src");
    ADevice* dst = new TestDevice("TestDevice Dst");

    CHECK_EQUAL(src->Initialize(), VC_SUCCESS);
    CHECK_EQUAL(dst->Initialize(), VC_SUCCESS);

    CHECK_EQUAL(pipe->c_str(), "Pipe 0");
    CHECK_EQUAL(src->c_str(), "TestDevice Src");
    CHECK_EQUAL(dst->c_str(), "TestDevice Dst");

    CHECK_EQUAL(pipe->DisconnectDevices(src, dst), VC_FAILURE);
    CHECK_EQUAL(pipe->ConnectDevices(src, NULL), VC_FAILURE);
    CHECK_EQUAL(pipe->ConnectDevices(NULL, NULL), VC_FAILURE);
    CHECK_EQUAL(pipe->ConnectDevices(NULL, dst), VC_FAILURE);
    CHECK_EQUAL(pipe->ConnectDevices(src, dst), VC_SUCCESS);
    CHECK_EQUAL(pipe->ConnectDevices(src, dst), VC_FAILURE);
    CHECK_EQUAL(pipe->DisconnectDevices(src, dst), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectDevices(dst, src), VC_FAILURE);
    CHECK_EQUAL(pipe->DisconnectDevices(NULL, NULL), VC_FAILURE);
    CHECK_EQUAL(pipe->ConnectDevices(src, dst), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectDevices(src, dst), VC_SUCCESS);

    CHECK_EQUAL(src->Uninitialize(), VC_SUCCESS);
    CHECK_EQUAL(dst->Uninitialize(), VC_SUCCESS);

    delete pipe;
    delete src;
    delete dst;
}

TEST(InputOutputPorts)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing InputOutputPorts\n"));
    BasePipe* pipe = new BasePipe("InputOutput Pipe 0");
    InputPort* input = new InputPort("InputOutput Input 0", NULL);
    OutputPort* output = new OutputPort("InputOutput Output 0", NULL);

    CHECK_EQUAL(pipe->c_str(), "InputOutput Pipe 0");
    CHECK_EQUAL(input->c_str(), "InputOutput Input 0");
    CHECK_EQUAL(output->c_str(), "InputOutput Output 0");

    CHECK_EQUAL(pipe->ConnectPorts(input, output), VC_SUCCESS);
    CHECK_EQUAL(pipe->ConnectPorts(input, output), VC_FAILURE);
    Buffer* buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->WriteData((void* )"RPiCast", 7), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);
    Buffer* buf2 = input->GetFilledBuffer();
    CHECK(!!buf2);
    CHECK(!memcmp(buf2->GetData(), "RPiCast", 7));
    CHECK_EQUAL(input->RecycleBuffer(buf2), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(input, output), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(input, output), VC_FAILURE);

    delete pipe;
    delete input;
    delete output;
}

TEST(FileIOTEST)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing FileIOTEST\n"));
    BasePipe* pipe = new BasePipe("FileIO Pipe 0");
    ADevice* fsink = new FileSink("FileIO FileSink", "FileSink.out");
    ADevice* fsrc = new FileSrc("FileIO FileSrc", "FileSink.out");
    OutputPort* output = new OutputPort("FileIO Output 0", NULL);
    InputPort* input = new InputPort("FileIO Input 0", NULL);
    Buffer* buf;

    fsink->Initialize();
    fsrc->Initialize();

    CHECK_EQUAL(pipe->c_str(), "FileIO Pipe 0");
    CHECK_EQUAL(fsink->c_str(), "FileIO FileSink");
    CHECK_EQUAL(fsrc->c_str(), "FileIO FileSrc");
    CHECK_EQUAL(output->c_str(), "FileIO Output 0");
    CHECK_EQUAL(input->c_str(), "FileIO Input 0");

    CHECK_EQUAL(pipe->ConnectPorts(fsink->Input(0), output), VC_SUCCESS);
    CHECK_EQUAL(pipe->ConnectPorts(input, fsrc->Output(0)), VC_SUCCESS);

    CHECK_EQUAL(fsink->SendCommand(VC_CMD_START), VC_SUCCESS);
    usleep(100000); //wait for thread to start
    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->SetTag(TAG_START), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);

    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->WriteData((void* )"RPiCast", 7), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);

    buf = output->GetBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->SetTag(TAG_END), VC_SUCCESS);
    CHECK_EQUAL(output->PushBuffer(buf), VC_SUCCESS);
    usleep(100000); // wait for fsink to process buffer
    CHECK_EQUAL(fsink->SendCommand(VC_CMD_STOP), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(fsink->Input(0), output), VC_SUCCESS);
    fsink->Uninitialize();
    delete fsink;

    CHECK_EQUAL(fsrc->SendCommand(VC_CMD_START), VC_SUCCESS);

    //wait untill a buffer is pushed but source device
    while (!input->IsBufferAvailable());
    CHECK(input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK_EQUAL(buf->GetTag(), TAG_START);
    CHECK_EQUAL(input->RecycleBuffer(buf), VC_SUCCESS);

    //wait untill a buffer is pushed by source device
    while (!input->IsBufferAvailable());
    CHECK(input->IsBufferAvailable());
    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "RPiCast", 7));
    CHECK_EQUAL(fsrc->SendCommand(VC_CMD_STOP), VC_SUCCESS);
    CHECK_EQUAL(pipe->DisconnectPorts(input, fsrc->Output(0)), VC_SUCCESS);
    fsrc->Uninitialize();
    delete fsrc;

    FILE* fp;
    char c[12];
    fp = fopen("FileSink.out", "rb");
    CHECK_EQUAL(fread(c, 1, 7, fp), (uint32_t )7);
    CHECK(!memcmp(c, "RPiCast", 7));

    fclose(fp);
    delete pipe;
    delete output;
    delete input;

}

TEST(SocketIOTest)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing SocketIOTest\n"));
    InputPort* input = new SocketInput("Socket Input", NULL, "127.0.0.1", 9000);
    OutputPort* output1 = new SocketOutput("Socket Output1", NULL, "127.0.0.1", 9000);

    Buffer* buf = output1->GetBuffer();
    CHECK(!!buf);
    buf->WriteData((void*) "RPiCast Test1", 13);
    output1->PushBuffer(buf);
    buf = NULL;

    while (!input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "RPiCast Test1", 13));
    input->RecycleBuffer(buf);
    buf = NULL;

    delete output1;

    while (!input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "Disconnect", buf->GetSize()));
    input->RecycleBuffer(buf);
    buf = NULL;

    OutputPort* output2 = new SocketOutput("Socket Output2", NULL, "127.0.0.1", 9000);
    buf = output2->GetBuffer();
    CHECK(!!buf);
    buf->WriteData((void*) "RPiCast Test2", 13);
    output2->PushBuffer(buf);

    while (!input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "RPiCast Test2", buf->GetSize()));
    input->RecycleBuffer(buf);
    buf = NULL;

    delete output2;
    delete input;
}

TEST(SocketIODeviceTest)
{
    DBGPRINT(LEVEL_ALWAYS, ("Testing SocketIODeviceTest\n"));

    BasePipe* pipe = new AVPipe("AvPipe");
    ADevice* server = pipe->GetDevice(VC_SOCKET_RECEIVER, "SocketReceiver");
    ADevice* client = pipe->GetDevice(VC_SOCKET_TRANSMITTER, "SocketTransmitter", "127.0.0.1");

    InputPort* input = new InputPort("Input", NULL);
    OutputPort* output = new OutputPort("Output", NULL);

    server->Initialize();
    client->Initialize();

    pipe->ConnectPorts(client->Input(), output);
    pipe->ConnectPorts(input, server->Output());

    CHECK_EQUAL(server->SendCommand(VC_CMD_START), VC_SUCCESS);
    CHECK_EQUAL(client->SendCommand(VC_CMD_START), VC_SUCCESS);

    Buffer* buf = output->GetBuffer();
    CHECK(!!buf);
    buf->WriteData((void*) "RPiCast Test1", 13);
    output->PushBuffer(buf);
    buf = NULL;

    while (!input->IsBufferAvailable());

    buf = input->GetFilledBuffer();
    CHECK(!!buf);
    CHECK(!memcmp(buf->GetData(), "RPiCast Test1", 13));
    input->RecycleBuffer(buf);
    buf = NULL;

    client->SendCommand(VC_CMD_STOP);
    server->SendCommand(VC_CMD_STOP);

    pipe->DisconnectPorts(client->Input(), output);
    pipe->DisconnectPorts(input, server->Output());

    server->Uninitialize();
    client->Uninitialize();

    delete client;
    delete server;
    delete input;
    delete output;
}
}
int main(int argc, char* argv[])
{
    DebugSetLevel(1);
    int c;
    while ((c = getopt(argc, argv, "sf?l:d:t:")) != -1)
    {
        switch (c)
        {
        case 'd':
            DebugSetLevel(strtol(optarg, NULL, 10));
            break;
        default:
            break;
        }
    }

    return (UnitTest::RunAllTests());
}
