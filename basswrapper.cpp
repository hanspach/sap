#include "basswrapper.h"

BassErrorMessages* BassErrorMessages::instance = NULL;

BassErrorMessages::BassErrorMessages()
{
    errors[0] = "All is OK";
    errors[1] = "Memory error";
    errors[2] = "Can't open the file";
    errors[3] = "Can't find a free/valid driver";
    errors[4] = "The sample buffer was lost";
    errors[5] = "Invalid handle";
    errors[6] = "Unsupported sample format";
    errors[7] = "Invalid playback position";
    errors[8] = "BASS_Init has not been successfully called";
    errors[9] = "BASS_Start has not been successfully called";
    errors[12] = "No CD in drive";
    errors[13] = "Invalid track number";
    errors[14] = "Already initialized/paused/whatever";
    errors[16] = "Not paused";
    errors[17] = "Not an audio track";
    errors[18] = "Can't get a free channel";
    errors[19] = "An illegal type was specified";
    errors[20] = "An illegal parameter was specified";
    errors[21] = "No 3D support";
    errors[22] = "No EAX support";
    errors[23] = "Illegal device number";
    errors[24] = "Not playing";
    errors[25] = "Illegal sample rate";
    errors[27] = "The stream is not a file stream";
    errors[29] = "No hardware voices available";
    errors[31] = "The MOD music has no sequence data";
    errors[32] = "No internet connection could be opened";
    errors[33] = "Couldn't create the file";
    errors[34] = "Effects are not available";
    errors[35] = "The channel is playing";
    errors[37] = "Requested data is not available";
    errors[38] = "The channel is a 'decoding channel'";
    errors[39] = "A sufficient DirectX version is not installed";
    errors[40] = "Connection timedout";
    errors[41] = "Unsupported file format";
    errors[42] = "Unavailable speaker";
    errors[43] = "Invalid BASS version (used by add-ons)";
    errors[44] = "Codec is not available/supported";
    errors[45] = "The channel/file has ended";
    errors[46] = "The device is busy (eg. in 'exclusive' use by another process)";
    errors[-1] = "Some other mystery error";
    errors[1000] = "BassWma: the file is protected";
    errors[1001] = "BassWma: WM9 is required";
    errors[1002] = "BassWma: access denied (user/pass is invalid)";
    errors[1003] = "BassWma: no appropriate codec is installed";
    errors[1004] = "BassWma: individualization is needed";
    errors[2000] = "BassEnc: ACM codec selection cancelled";
    errors[2100] = "BassEnc: Access denied (invalid password)";
    errors[3000] = "BassVst: the given effect has no inputs and is probably a VST instrument and no effect";
    errors[3001] = "BassVst: the given effect has no outputs";
    errors[3002] = "BassVst: the given effect does not support realtime processing";
    errors[5000] = "BASSWASAPI: no WASAPI available";
    errors[6000] = "BASS_AAC: non-streamable due to MP4 atom order ('mdat' before 'moov')";
}

QString BassErrorMessages::string(const int& number)
{
    if(!instance)
        instance = new BassErrorMessages();
    return instance->errors.value(number);
}

DeviceInfo::DeviceInfo(const BASS_DEVICEINFO &info)
{
    name = QString::fromLatin1(info.name);    //fromUtf8(info.name);
    device = QString::fromUtf8(info.driver);
    flags = info.flags;
}

DeviceInfo::DeviceInfo(const QString &_name, const QString &_device, const bool& _active):
    name(_name), device(_device), active(_active)
{
}

BassWrapper* obj = NULL; //wird nur hier gebraucht!!!!

void CALLBACK StatusProc(const void *buffer, DWORD length, void *user)
{
   if (buffer && !length && (DWORD)user == obj->request){ // got HTTP/ICY tags, and this is still the current request
      qDebug() << buffer << ":" << length; // display status
    }
}

void CALLBACK MetaSync(HSYNC /*handle*/, DWORD /*channel*/, DWORD /*data*/, void* /*user*/)
{
    obj->doMeta();
}

void CALLBACK EndSync(HSYNC /*handle*/, DWORD /*channel*/, DWORD /*data*/, void* /*user*/)
{
    qDebug() << " from EndSync: ";

}

const int BassWrapper::InitState = 1001;
const int BassWrapper::ErrorState= 1000;
const int BassWrapper::PausedState = BASS_ACTIVE_PAUSED;
const int BassWrapper::PlayingState= BASS_ACTIVE_PLAYING;
const int BassWrapper::StoppedState=BASS_ACTIVE_STOPPED;

BassWrapper::BassWrapper(QWidget* parent)
{
    parentWindow = parent;
    checkPulseAudio();
    init();
    connect(&timer, &QTimer::timeout,this,&BassWrapper::elapsedTime);
    obj = this;
}

BassWrapper::~BassWrapper()
{
    BASS_Free();
}

bool BassWrapper::init(int device)
{
    BOOL result = FALSE;
#if defined(WIN32)
    result = BASS_Init(device,44100,0,(HWND)parentWindow->winId(),NULL);
#else
    result = BASS_Init(device,44100,0,0,NULL);
#endif
    if(!result) {
        QMessageBox::warning(parentWindow,"Simple Audio Player",
            tr("Couldn't launch BASS_Init. Reason: ") +
            BassErrorMessages::string(BASS_ErrorGetCode()));
        qApp->closeAllWindows();
    }
    currentState = StoppedState;
    currentStream = 0;
    BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT,TRUE);   // default device
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM,3000);   // max volume / 3
    BASS_SetConfig(BASS_CONFIG_UNICODE,TRUE);
    if(!BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST,1)) // enable playlist processing
        qDebug() << "Init:BASS_CONFIG_NET_PLAYLIST " << BassErrorMessages::string(BASS_ErrorGetCode());
    if(!BASS_SetConfig(BASS_CONFIG_NET_PREBUF,0)) // minimize automatic pre-buffering,
       qDebug() << "Init:BASS_CONFIG_NET_PREBUF " << BassErrorMessages::string(BASS_ErrorGetCode());
                                           // so we can do it(and display it)instead
    BASS_ChannelSetSync(currentStream,BASS_SYNC_META,0,&MetaSync,0); // Shoutcast
    BASS_ChannelSetSync(currentStream,BASS_SYNC_OGG_CHANGE,0,&MetaSync,0); // Icecast/OGG
    BASS_ChannelSetSync(currentStream,BASS_SYNC_END,0,&EndSync,0); // set syn

    return false;
}

bool BassWrapper::prepareFileStream(const QString& path) {
    HSTREAM str = 0;
    if(timer.isActive()) {
        timer.stop();
    }
#if defined(WIN32)
    const wchar_t* file = path.toStdWString().c_str();
    str=BASS_StreamCreateFile(FALSE,file,0,0,0);
#else
    const char* file = path.toStdString().c_str();
    str=BASS_StreamCreateFile(FALSE,file,0,0,0);
#endif
    if (str) {
        stop();
        currentStream = str;
        mediaLength = BASS_ChannelGetLength(currentStream,BASS_POS_BYTE);
        if(mediaLength != (quint64)-1) {
            timer.start(50);
            QString lenStr = formatNumber(mediaLength);
            emit mediaLen(lenStr);
        }
        emit metaData(path);
        play();
    }
    else {
        doStatus(ErrorState,"BASS_StreamCreateFile(): file:"
            + path + BassErrorMessages::string(BASS_ErrorGetCode()));
        return false;
    }
    return true;
}

bool BassWrapper::prepareMedia(const QString& url) {
    mediaLength = -1;
    request = 0;
    unsigned int r = ++request;
    if(timer.isActive())
        timer.stop();       // stop prebuffer monitoring
    stop();
    unsigned int c = BASS_StreamCreateURL(url.toStdString().c_str(),0,BASS_STREAM_BLOCK|
        BASS_STREAM_STATUS|BASS_STREAM_AUTOFREE,StatusProc,(void*)r); // open URL
    if (r != request) { // there is a newer request, discard this stream
        if (c)
            BASS_StreamFree(c);
        doStatus(ErrorState, "A new req hasaccured!");
        return false;
    }
    currentStream = c;
    if (!currentStream) { // failed to open
        doStatus(ErrorState, "not playing");

     } else {
        filePositionOk = false;
        timer.start(50); // start prebuffer monitoring
     }
    return true;
}

void BassWrapper::elapsedTime() {
    static int count = 0;

    if(isNetMedia && !filePositionOk) {
        unsigned int progress = BASS_StreamGetFilePosition(currentStream, BASS_FILEPOS_BUFFER)
            *100/BASS_StreamGetFilePosition(currentStream,BASS_FILEPOS_END); // percentage of buffer filled

        if (progress > 75 || !BASS_StreamGetFilePosition(currentStream,BASS_FILEPOS_CONNECTED)) { // over 75% full (or end of download)
            filePositionOk = true;
            doMeta();
            /*
            BASS_ChannelSetSync(currentStream,BASS_SYNC_META,0,&MetaSync,0); // Shoutcast
            BASS_ChannelSetSync(currentStream,BASS_SYNC_OGG_CHANGE,0,&MetaSync,0); // Icecast/OGG
            BASS_ChannelSetSync(currentStream,BASS_SYNC_END,0,&EndSync,0); // set sync for end of stream
            */
            play();
        }
        else {
            doStatus(InitState, "In progress...");
        }
    }
    if(++count == 20) { // per second
        count = 0;
        unsigned long long pos = BASS_ChannelGetPosition(currentStream,BASS_POS_BYTE);
        if(currentState == PlayingState && pos != (unsigned long long)-1) {
           QString posStr = formatNumber(pos);
           int percent = 0;
           if(mediaLength != (unsigned long long)-1) {
               double quot = ((double)pos / mediaLength) * 100;
               percent = (int)quot;
               if(mediaLength - pos < 1000) {
                   timer.stop();
                   emit mediaEnd();
               }
           }
           emit mediaPos(posStr, percent);
        }
    }
}

bool BassWrapper::play()
{
    if(currentStream) {
        if(!BASS_ChannelPlay(currentStream,FALSE)) {
            doStatus(currentState=ErrorState, "BASS_ChannelPlay(): " +
                BassErrorMessages::string(BASS_ErrorGetCode()));
            return false;
        } else {
            doStatus(currentState=PlayingState);
        }
    }
    return false;
}

bool BassWrapper::pause()
{
    if(currentStream) {
        bool res = BASS_ChannelPause(currentStream) != 0;
        if(res) {
           doStatus(currentState=PausedState);
        } else {
           doStatus(currentState=ErrorState, "BASS_ChannelPause(): " +
                BassErrorMessages::string(BASS_ErrorGetCode()));
        }
        return res;
    }
    return false;
}

bool BassWrapper::stop()
{
    if(currentState == PlayingState || currentState == PausedState)
        BASS_ChannelStop(currentStream);
    if(currentStream) {
        BASS_StreamFree(currentStream);      // close old stream
        currentStream = 0;
        currentState = StoppedState;
    }
    return false;
}

void BassWrapper::doMeta()
{
    QString s, res = QString();
    const char *meta= BASS_ChannelGetTags(currentStream,BASS_TAG_META);
    if(meta) {
        s = QString::fromLatin1(meta);   //fromUtf8(meta);
        QStringList tokens = s.split(";");
        for(int i=0; i < tokens.length(); i++) {
            QStringList parts = tokens.at(i).split("'");
            if(parts.length() == 3) {
                res += parts[1];
                if(i < tokens.length()-1)
                    res += "   ";
            }
        }
    }  else {
        meta = BASS_ChannelGetTags(currentStream,BASS_TAG_OGG);
        if (meta) {
           s = QString::fromLatin1(meta);    //fromUtf8(meta);
           QStringList tokens = s.split("=");
           for(int i=1; i < tokens.length(); i +=2) {
               res += tokens[i];
               if(i < tokens.length()-1)
                   res += "   ";
           }
        }
    }
    emit metaData(res);
}

void BassWrapper::doStatus(const int &status, const QString &reason) {
    emit metaStatus(status, reason);
}

QList<DeviceInfo> BassWrapper::devices()
{
    devicesInfo.clear();
    BASS_DEVICEINFO info;
    for (int a=1; BASS_GetDeviceInfo(a, &info); a++) {
        if ((info.flags & BASS_DEVICE_ENABLED)== BASS_DEVICE_ENABLED){
           devicesInfo.append(DeviceInfo(info));
        }
    }
    return devicesInfo;
}

QList<DeviceInfo> BassWrapper::devices(const QFile &file)
{
    devicesInfo.clear();
    QByteArray data = QByteArray();
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(file.fileName(),QStringList() << "list-sinks");
    if(process.waitForStarted()) {
        while(process.waitForReadyRead())
            data.append(process.readAll());
    }
    if(!data.isEmpty()) {
        QList<QByteArray> rows = data.split('\n');
        QString name = QString();
        QString description = QString();
        int first, last;
        bool active = false;

        foreach(QByteArray row, rows) {
            QString line(row);
            if(line.contains("name:")) {
                first = line.indexOf('<');
                last  = line.lastIndexOf('>');
                if(first != -1 && last > first) {
                    name = line.mid(first+1,last-first-1);
                }
            }
            if(line.contains("state:")) {
                if(line.contains("RUNNING"))
                    active = true;
                else
                    active = false;
            }
            if(line.contains("device.description")) {
                first = line.indexOf('"');
                last  = line.lastIndexOf('"');
                if(first != -1 && last > first) {
                    description = line.mid(first+1,last-first-1);
                }
            }
            if(!name.isEmpty() && !description.isEmpty()) {
                DeviceInfo info(description,name, active);
                devicesInfo.append(info);
                name = QString();
                description = QString();
                active = false;
            }
        }
    }
    return devicesInfo;
}

int BassWrapper::device() const
{
    return BASS_GetDevice();
}

bool BassWrapper::setDevice(int device)
{
    BASS_Free();
    init(device);
    if(BASS_SetDevice(device) == FALSE) {
         qDebug() << "BASS_SetDevice(): " << BassErrorMessages::string(BASS_ErrorGetCode());
    }
    return false;
}

int BassWrapper::volume()
{
    int res = -1;
    float vol = BASS_GetVolume();
    if(vol != -1)
        res = vol * 100;
    else
        qDebug() << "BASS_GetVolume(): " << BassErrorMessages::string(BASS_ErrorGetCode());
    return res;
}

bool BassWrapper::setVolume(const unsigned int& vol)
{
    float value = vol / 100.0f;
    if(BASS_SetVolume(value) == 0) {
        qDebug() << "BASS_SetVolume(): " << BassErrorMessages::string(BASS_ErrorGetCode());
        return false;
    }
    return true;
}

int BassWrapper::state() const
{
    return currentState;
}

void BassWrapper::checkPulseAudio()
{
    QFile file("/usr/bin/pulseaudio");
    if(file.exists()) {
        QStringList list;
        list << "--check";
        QProcess process;
        process.start(file.fileName(),list);
        process.waitForFinished(100);
        if(process.exitStatus()==0 && process.exitCode()!=0) {
            list.replace(0, "--start");
            QProcess::startDetached(file.fileName(), list);
        }
    }
}

QString BassWrapper::formatNumber(quint64 number)
{
    unsigned int second = number / 176557; //maybe wrong?
    unsigned int min = second / 60;
    unsigned int sec = second % 60;
    QString strm = QString("%1").arg(min,2,10,QLatin1Char('0'));
    QString strs = QString("%1").arg(sec,2,10,QLatin1Char('0'));
    QString res = strm + ":" + strs;
    return res;
}

QList<DeviceInfo> BassWrapper::getDevicesInfo() const
{
    return devicesInfo;
}

QString BassWrapper::deviceFlagsToString(uint flags) const
{
    QString res = QString();
    if((flags & BASS_DEVICE_ENABLED)==BASS_DEVICE_ENABLED) res += "BASS_DEVICE_ENABLED\n";
    if((flags & BASS_DEVICE_DEFAULT)==BASS_DEVICE_DEFAULT) res += "BASS_DEVICE_DEFAULT\n";
    if((flags & BASS_DEVICE_INIT)==BASS_DEVICE_INIT) res += "BASS_DEVICE_INIT\n";
    if((flags & BASS_DEVICE_TYPE_DIGITAL)==BASS_DEVICE_TYPE_DIGITAL) res += "BASS_DEVICE_TYPE_DIGITAL\n";
    if((flags & BASS_DEVICE_TYPE_DISPLAYPORT)==BASS_DEVICE_TYPE_DISPLAYPORT)	res += "BASS_DEVICE_TYPE_DISPLAYPORT\n";
    if((flags & BASS_DEVICE_TYPE_HANDSET)==BASS_DEVICE_TYPE_HANDSET)	res +="BASS_DEVICE_TYPE_HANDSET\n";
    if((flags & BASS_DEVICE_TYPE_HDMI) == BASS_DEVICE_TYPE_HDMI) res +="BASS_DEVICE_TYPE_HDMI\n";
    if((flags & BASS_DEVICE_TYPE_HEADPHONES)==BASS_DEVICE_TYPE_HEADPHONES)	res +="BASS_DEVICE_TYPE_HEADPHONES\n";
    if((flags & BASS_DEVICE_TYPE_HEADSET)==BASS_DEVICE_TYPE_HEADSET) res +="BASS_DEVICE_TYPE_HEADSET\n";
    if((flags & BASS_DEVICE_TYPE_LINE)==BASS_DEVICE_TYPE_LINE)	res +="BASS_DEVICE_TYPE_LINE\n";
    if((flags & BASS_DEVICE_TYPE_MICROPHONE)==BASS_DEVICE_TYPE_MICROPHONE)	res +="BASS_DEVICE_TYPE_MICROPHONE\n";
    if((flags & BASS_DEVICE_TYPE_NETWORK)==BASS_DEVICE_TYPE_NETWORK) res +="BASS_DEVICE_TYPE_NETWORK\n";
    if((flags & BASS_DEVICE_TYPE_SPDIF)==BASS_DEVICE_TYPE_SPDIF) res +="BASS_DEVICE_TYPE_SPDIF\n";
    if((flags & BASS_DEVICE_TYPE_SPEAKERS)==BASS_DEVICE_TYPE_SPEAKERS) res += "BASS_DEVICE_TYPE_SPEAKERS\n";
    res += "\n";
    return res;
}

