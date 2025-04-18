/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef OCTAVEINTERFACE_H
#define OCTAVEINTERFACE_H

#include <QObject>
#include <QSharedDataPointer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <octave.h>
#include <ovl.h>
#include <octavews.h>

#undef OCTAVE_THREAD



class octaveInterface : public QObject
{
    Q_OBJECT
private:
    QStringList             commandList;
    QString                 commandCurrent;
    QString                 last_output;
    bool                    bStopThread;
    QMutex                  sync;
    octave::interpreter     *_octave_engine; // This is the MAIN _octave_engine (only one _octave_engine)
    octavews                *_workspace;
    //std::set<std::string>   _vars_immediate_update;
    std::pair<std::string, std::string> _var_immediate_update;      // This is the variable / filename associated with the pipe
#ifndef OCTAVE_THREAD
    bool                    _b_running_command; // true if we are running a command from the command line
    bool                    _b_running_script;
    int                     _stopped_at;
#endif
    //------------------------------------------------------------------------
    std::shared_ptr<class octaveScript> _current_script;
    std::string                         _immediate_filename;
	QString								_last_error;
	QString								_last_warning;
public:
	static octaveInterface*	_octave_interface_instance;

    octaveInterface();
    ~octaveInterface();
    // Commands from in-line command editor
    bool                    appendCommand(const QString &strCommand);
    const QString &         getCurrentCommand();
    bool                    retrieveCommand(QString &commandToExecute);
    void                    completeCommand();
    void                    clearCommandList();
    const QString &         get_last_output() {return last_output;}
    // Command execution status
    bool                    isReady();
    void                    stopThread(bool bStop=true);
    // Main octave engine
    inline octave::interpreter*    get_octave_engine() {return _octave_engine;}
    void                    run(std::shared_ptr<class octaveScript> script);
    bool                    run(const QString &program, bool remove_immediate_error_msg = false);
    void                    set_pwd(const QString& path);
/*----------------------------------------*/
	void					set_last_error_message(QString error = "");
 /*--------------------------------------------
  * Workspace commands
  * ------------------------------------------*/
    void            clearWorkspace();
	//bool            appendVariable();
    QString         appendVariable(QString name, const octave_value& val, bool internal, bool toOctave=false, QStringList indep=QStringList(), QStringList dep=QStringList());

    void            refreshWorkspace();
    octave_value_list
                    execute_feval(QString command, octave_value_list& in, int n_outputs);
    int             findFunc(QString funcName,bool wait);

    void            append_variable(QString name, const octave_value& val, bool internal= false);
    void            refresh_workspace();
    octavews*       get_workspace() {return _workspace;}
    void            update_interpreter_internal_vars();
    void            execute_feval(QString command, const string_vector& input, const string_vector& output); // NB we may have empty in some output var
    bool            save_workspace_to_file(QString filename);
    void            immediate_update_of_radar_var(const std::string& str, const std::string& filename);
    void            immediate_inquiry_of_radar_var(const std::string& str, const std::string& filename);
    void            immediate_command(const std::string& str, const std::string& filename);
    void            remove_interface_file();
signals:
    void            workerError(QString error);
    void            workspaceUpdated();
    void            workspaceDeleted();
    void            workspaceCleared();
    void            workspaceVarAdded();
    void            workspaceVarModified();
    void            workspaceVarDeleted();
    void            errorWhileRunning();
#ifndef OCTAVE_THREAD
    void            commandCompleted(QString command, int errorcode);
#endif
    void            updatedVariable(const std::string& var);
    void            updatedVariables(const std::set<std::string>& vars);
    void            immediateUpdateVariable(const std::string& var);
    void            inquiryVariable(const std::string& var);
    void            sendCommand(const std::string& var);

};


#ifdef OCTAVE_THREAD
class qDataThread : public QObject
{
private:

    QMutex          sync;
    QWaitCondition  pauseCond;
    bool            bPause;
    octaveInterface    *data;
    Q_OBJECT
public:

public slots:
    void processDataThread();
    void finishedDataThread();
signals:
    void error(QString err);
    void finished();
    void workspaceUpdated();
    void commandCompleted(QString command, int errorcode);
    void commandStarting(QString command);
    void fifoEmpty();

public:
    explicit qDataThread(QObject * parent = 0);
    ~qDataThread();

    octaveInterface* getData() {return data;}

    void resume();
    void pause();
    bool isPaused() {return bPause;}
    bool isReady()  {return data==NULL?true:data->isReady();}

    void runFile(QString fileName, const octave_value_list& args = octave_value_list (),
                 int nargout = 0);


    int findVar(const char* varname);
    int findFunc(QString funcName,bool wait=false);
};
#endif
#endif // OCTAVEINTERFACE_H
