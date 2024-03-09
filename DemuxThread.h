#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

class DemuxThread
{
public:
    DemuxThread();

public:
    int loadFile();

    int loop();
};

#endif // DEMUXTHREAD_H
