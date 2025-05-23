
/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "octaveinterface.h"
#include <octave.h>
#include <oct.h>
#include <parse.h>
#include "octavescript.h"
#include <QFileInfo>
#include <QCoreApplication>
//#include "iostream"


octaveInterface*	octaveInterface::_octave_interface_instance = nullptr;
/*-----------------------------
 * QSharedData implementation
 *----------------------------*/


octaveInterface::octaveInterface() :
	commandList()
	, commandCurrent("")
	, bStopThread(false)
	, _workspace(nullptr)
	, _immediate_filename("")
{
	// Initialize Octave Interpreter
	_octave_engine = new octave::interpreter();
	_octave_engine->interactive(true);
	_octave_engine->initialize();
	_octave_engine->initialize_history(true);
	_octave_engine->initialize_load_path();

	_octave_engine->get_error_system().debug_on_caught(false);
	_octave_engine->execute();

	octave::output_system& os = _octave_engine->get_output_system();

	os.flushing_output_to_pager(false);
	os.page_screen_output(false);

	_workspace = new octavews(_octave_engine);
	_workspace->data_interface(this);
	_current_script = nullptr;

#ifndef OCTAVE_THREAD
	_b_running_command = false;
	_b_running_script = true;
#endif
	// These dummy declarations allow for proper linking of the radarparameter template.
	// to be investigated.
	{ Array<float> x(dim_vector({1,1})); x(0)=1;}
	{ Array<int8_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<int16_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<int32_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<uint8_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<uint16_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<uint32_t> x(dim_vector({1,1})); x(0)=1;}
	{ Array<char> x(dim_vector({1,1})); x(0)=0x01;}

	// Error handlers
	_octave_engine->get_error_system().backtrace_on_warning(true);
	_octave_engine->get_error_system().initialize_default_warning_state();


	QString currentPath = QCoreApplication::applicationDirPath();
	// Add General
	try
	{
		_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(currentPath.toUtf8())}));
	}
	catch(...)
	{

	}


	QStringList required_paths={
		"help","io","linear-algebra","miscellaneous","path","set","specfun","strings","time","statistics","polynomial","pkg","pkg/private","plot/util","general","elfun","geometry","image","ode"
	};

	for (auto& path: required_paths)
	{
#ifdef WIN32
		QString	octavePath  = QFileInfo(currentPath, QString("../share/octave/9.4.0/m/")+path+"/").absolutePath()+"/";
#else
		QString	octavePath  = QFileInfo(currentPath, QString("/usr/share/octave/9.4.0/m/")+path+"/").absolutePath()+"/";
#endif
		try
		{
			_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));
		}
		catch(...)
		{

		}
	}
#ifdef WIN32
	try
	{

		QString octavePath  = QFileInfo(currentPath, QString("../lib/octave/9.4.0/site/oct/x86_64-w64-mingw32/")).absolutePath();
		_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));
		octavePath  = QFileInfo(currentPath, QString("../lib/octave/site/oct/api-v59/x86_64-w64-mingw32/")).absolutePath();
		_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));
		octavePath  = QFileInfo(currentPath, QString("../lib/octave/site/oct/x86_64-w64-mingw32/")).absolutePath();
		_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));

		octavePath  = QFileInfo(currentPath, QString("../lib/octave/9.4.0/oct/x86_64-w64-mingw32/")).absolutePath();
		_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));
	}
	catch(...)
	{
	}
#endif
	// Toolboxes
	QString pkgPrefix = QFileInfo(currentPath, QString("../toolboxes/")).absolutePath();
	if (!QDir(pkgPrefix).exists())
		QDir().mkdir(pkgPrefix);

	try
	{
#ifdef WIN32
		QString xtmp= (QDir().toNativeSeparators(pkgPrefix)+QDir().separator());
		//xtmp = xtmp.last(xtmp.length()-2);
		std::list<octave_value> prefix({
										charNDArray(QString("prefix").toUtf8()),
										charNDArray(xtmp.toUtf8())});
#else
		std::list<octave_value> prefix({
										charNDArray(QString("prefix").toUtf8()),
										charNDArray((QDir().toNativeSeparators(pkgPrefix)+QDir().separator()).toUtf8())});
#endif


		_octave_engine->feval("pkg",octave_value_list(prefix));
	}
	catch(...)
	{
	}

	// Pre-load directories in "toolboxes"
	QDirIterator directories(pkgPrefix, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

	while(directories.hasNext()){
		directories.next();
		try
		{
			_octave_engine->feval("addpath", std::list<octave_value>({charNDArray(directories.filePath().toUtf8())}));
		}
		catch(...)
		{
		}
	}

}
//-----------------------------
octaveInterface::~octaveInterface()
{
	if (_octave_engine!=nullptr)
	{
		delete _octave_engine;
		_octave_engine= nullptr;
	}

	if (_workspace!=nullptr)
		delete _workspace;


}
//-----------------------------
bool octaveInterface::isReady()
{
	return commandList.isEmpty() && (_current_script==nullptr);
}
//-----------------------------
void octaveInterface::stopThread(bool bStop)
{
	bStopThread = bStop;
}
//-----------------------------
bool octaveInterface::appendCommand(const QString &newCommand)
{
	last_output = "";
	if (_b_running_command) return true;
#ifdef OCTAVE_THREAD
	sync.lock();
	commandList.append(newCommand);
	sync.unlock();
	return bStopThread;
#else
	int parse_status;

	_b_running_command = true;
	bool b_ok = true;
	try
	{
		octave::flush_stdout();
		update_interpreter_internal_vars();

		_octave_engine->eval_string(newCommand.toStdString(),false,parse_status);


	}
	catch (const octave::exit_exception& ex)
	{
		QString str_error = "";
		emit workerError(QString("Error while executing:\n")+newCommand+"\n"+str_error);

		parse_status = -2;
		b_ok = false;

	}
	catch (const octave::execution_exception& ex)
	{
		_octave_engine->get_error_system().save_exception(ex);
		QString str_error = QString::fromStdString(_octave_engine->get_error_system().last_error_message());
		emit workerError(QString("Error while executing:\n")+newCommand+"\n"+str_error);

		parse_status = -1;
		b_ok = false;
	}
	emit commandCompleted(newCommand,parse_status);
	if (b_ok)
		completeCommand();
	else
	{
		remove_interface_file();
		emit errorWhileRunning();
	}

	_b_running_command = false;
#endif
	return b_ok;
}
//-----------------------------
bool octaveInterface::retrieveCommand(QString &commandToExecute)
{
	sync.lock();
	if (bStopThread)
		commandCurrent = "";
	else
	{
		if (!commandList.isEmpty())
			commandCurrent = commandList.first();
		else
			commandCurrent = "";
	}
	commandToExecute = commandCurrent;
#ifdef OCTAVE_THREAD
	sync.unlock();
#endif
	return bStopThread;
}
//-----------------------------
void octaveInterface::clearCommandList()
{
	sync.lock();
	commandList.clear();
	sync.unlock();
}
//-----------------------------
void	octaveInterface::clearWorkspace()
{
	_octave_engine->clear_variables();
	_octave_engine->clear_global_variables();
	emit workspaceUpdated();
}

QString octaveInterface::appendVariable(QString name, const octave_value& val, bool internal, bool toOctave, QStringList indep, QStringList dep )
{
	return "";
}
//-----------------------------
void    octaveInterface::set_pwd(const QString& path)
{
	if (_octave_engine==nullptr) return;
	_octave_engine->chdir(path.toStdString());
}
//-----------------------------
void octaveInterface::completeCommand()
{
#ifdef OCTAVE_THREAD
	sync.lock();
#endif
	if (!commandList.isEmpty())
		commandList.removeFirst();

	if (_workspace!=nullptr)
		_workspace->interpreter_to_workspace();

// emit workspaceUpdated();


#ifdef OCTAVE_THREAD
	sync.unlock();
#endif
}
//-----------------------------
octave_value_list octaveInterface::execute_feval(QString command, octave_value_list& in, int n_outputs)
{

	return _octave_engine->feval(command.toLatin1(),in,n_outputs);
}


int octaveInterface::findFunc(QString funcName,bool wait)
{
	int retvalue=0;
#ifdef OCTAVE_THREAD
	if (!wait)
		if (!sync.tryLock())
			return 0;

	if (wait)
		sync.lock();
#endif
	octave_value_list toFind(2);
	toFind(0)=funcName.toStdString();
	toFind(1)="builtin";
	octave_value_list ret = _octave_engine->feval("exist",toFind,1);
	retvalue = ret(0).int_value();
	sync.unlock();
	return retvalue;
}


void octaveInterface::run(std::shared_ptr<octaveScript> script)
{
	if (_b_running_command)
		return;

	if (script == nullptr)
		return;

	if (_octave_engine==nullptr)
		return;


	if (script->has_breakpoints())
	{
		//int nline = 0;
	}

	for (int n=0; n < script->get_line_count(); n++)
	{
		//int bOk;
		QString current_line = script->get_line(n);
		appendCommand(current_line);
	}
}

bool    octaveInterface::run(const QString &program, bool remove_immediate_error_msg)
{
	if (_b_running_command)
		return false;

	if (_octave_engine==nullptr)
		return false;

	try
	{
		update_interpreter_internal_vars();

		_octave_engine->eval(program.toStdString(),0);

	}
	catch (const octave::exit_exception& ex)
	{
		if (!remove_immediate_error_msg)
			emit workerError(QString("Octave exit-exception while executing current script\n")+QString::number(ex.exit_status()));
		//fi.remove();
		_b_running_command = false;
		return false;
	}
	catch (const octave::execution_exception& ex)
	{
		_octave_engine->get_error_system().save_exception(ex);
		QString str_error = QString::fromStdString(_octave_engine->get_error_system().last_error_message());

		if (!remove_immediate_error_msg)
			emit workerError(QString("Octave execution exception while executing current script\n")+str_error);
		//fi.remove();
		_b_running_command = false;
		return false;
	}

	completeCommand();
	//fi.remove();

	_b_running_command = false;
	emit workspaceUpdated();
	return true;
}


void            octaveInterface::append_variable(QString name, const octave_value& val, bool internal)
{
	if (_workspace == nullptr) return;
	if (name.isEmpty()) return;
	_workspace->add_variable(name.toStdString(), !internal, val);
	if (internal)
		_workspace->workspace_to_interpreter();
}


void            octaveInterface::refresh_workspace()
{
	if (_workspace == nullptr) return;
	_workspace->workspace_to_interpreter();
}

void            octaveInterface::update_interpreter_internal_vars()
{
	if (_workspace==nullptr) return;
	_workspace->workspace_to_interpreter();
}

void            octaveInterface::execute_feval(QString command, const string_vector& input, const string_vector& output)
{
	if (_workspace==nullptr) return;
	// Build inputs
	octave_value_list in = _workspace->get_var_values(input);
	int nout = output.numel();
	_workspace->set_var_values(output,_octave_engine->feval(command.toLatin1(),in,nout));
}

//---------------------------------------
bool    octaveInterface::save_workspace_to_file(QString filename)
{
	if (_workspace == nullptr) return false;
	_workspace->save_to_file(filename.toStdString());
	return true;
}
void octaveInterface::remove_interface_file()
{
	if (_immediate_filename.empty()) return;
	QFileInfo fi(QString::fromStdString(_immediate_filename));
	if (fi.fileName()!=".rdk_tmp.atp") return;
	std::remove(_immediate_filename.c_str());
	_immediate_filename.clear();
}
//-----------------------------
void    octaveInterface::immediate_update_of_radar_var(const std::string& str, const std::string& filename)
{
	_immediate_filename = filename;
	if ((str.empty())||(filename.empty())) {remove_interface_file(); return;}

	if (!std::filesystem::exists(filename)) return;
	//_vars_immediate_update.insert(str);
	emit immediateUpdateVariable(str);
	// Delete lock file
	remove_interface_file();
}

//-----------------------------
void    octaveInterface::immediate_inquiry_of_radar_var(const std::string& str, const std::string& filename)
{

	_immediate_filename = filename;
	if ((str.empty())||(filename.empty())) {remove_interface_file(); return;}

	// Add the variable to the list of modified vars
	//_vars_immediate_update.insert(str);
	if (!std::filesystem::exists(filename)) return;

	emit inquiryVariable(str);
	// Delete lock file
	remove_interface_file();
}
//-----------------------------
void    octaveInterface::immediate_command(const std::string& str, const std::string& filename)
{
	_immediate_filename = filename;
	if ((str.empty())||(filename.empty()))
		if ((str.empty())||(filename.empty())) {remove_interface_file(); return;}

	// Add the variable to the list of modified vars
	if (!std::filesystem::exists(filename)) return;

	emit sendCommand(str);
	// Delete lock file
	remove_interface_file();
}

/*-----------------------------
 * QWorker implementation
 *----------------------------*/
#ifdef OCTAVE_THREAD
qDataThread::qDataThread(QObject * parent ) :
	data(new octaveInterface())
{
}

//-----------------------------
void qDataThread::processDataThread()
{

	if (data->get_octave_engine()==nullptr)
	{
		emit error("Octave Interpreter not found");
		emit finished();
		return;
	}

	while (1)
	{
		QString strToExecute;
		bool bTerminate = data->retrieveCommand(strToExecute);
		if (bTerminate)
		{
			data->clearCommandList();
			break;
		}

		sync.lock();
		if (!strToExecute.isEmpty())
		{
			int parse_status=0;
			if (data->get_octave_engine()!=nullptr)
			{

				try
				{
					data->updateInterpreterInternalVariables();
					emit commandStarting(strToExecute);
					data->get_octave_engine()->eval_string(strToExecute.toStdString(),true,parse_status);
				}
				catch (const octave::exit_exception& ex)
				{
					parse_status = -2;
					emit error(QString("Octave interpreter exited with status = ")+ ex.exit_status ());
				}
				catch (const octave::execution_exception&)
				{
					parse_status = -1;
					emit error("error encountered in Octave evaluator!");
				}

				data->completeCommand();
				emit commandCompleted(strToExecute,parse_status);

			}
			else
				emit error("Octave Interpreter not found");
		}
		else
			emit fifoEmpty();
		sync.unlock();

		pause();
		sync.lock();
		if(bPause)
			pauseCond.wait(&sync); // in this place, your thread will stop to execute until someone calls resume
		sync.unlock();
	}

	emit finished();

}
//-----------------------------
void qDataThread::finishedDataThread()
{

}
//-----------------------------

qDataThread::~qDataThread()
{

	if (data!=nullptr)
		delete data;
	data = nullptr;

}

//-----------------------------
void qDataThread::resume()
{
	//sync.lock();
	bPause = false;
	//sync.unlock();
	pauseCond.wakeAll();
}
//-----------------------------
void qDataThread::pause()
{
	sync.lock();
	bPause= true;
	sync.unlock();
}
//-----------------------------
void qDataThread::runFile(QString fileName, const octave_value_list& args ,  int nargout)
{
	if (isPaused())
		resume();

}

//-----------------------------
int qDataThread::findFunc(QString funcName,bool wait)
{
	int retvalue=0;
	if (!wait)
		if (!sync.tryLock())
			return 0;

	if (wait)
		sync.lock();

	octave_value_list toFind(2);
	toFind(0)=funcName.toStdString();
	toFind(1)="builtin";
	octave_value_list ret = data->get_octave_engine()->feval("exist",toFind,1);
	retvalue = ret(0).int_value();
	sync.unlock();
	return retvalue;
}

#endif



