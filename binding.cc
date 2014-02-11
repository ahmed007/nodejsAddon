#include "binding.h"

using namespace v8;
using namespace node;



/************************** MACROs To be defined here *****************************/
#define CUSTOM_USER_DIRECTORY "/dev/null"
#define CUSTOM_PLUGIN_PATH     "/root"
#define UI_ID                  "testThread"
#define PLUGIN_SAVE_PREF       "/purple/nullclient/plugins/saved"
#define ARRAY_SIZE_FOR_PLUGIN  15 //magic number for plugin need to be changed
/**
 * The following eventloop functions are used in both pidgin and purple-text. If your
 * application uses glib mainloop, you can safely use this verbatim.
 */
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

/**
Global variables
*/
Local<Function> callback;
/************************************************************************************/



namespace purple {
Persistent<Function> purpleBindings::constructor;
static void
  Initialize(Handle<Object> target);

typedef struct _PurpleGLibIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleGLibIOClosure;

static void purple_glib_io_destroy(gpointer data)
{
	g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PurpleGLibIOClosure *closure ;
	PurpleInputCondition purple_cond=(PurpleInputCondition)0;
	
	closure->data=data;
	//purple_cond = 0;

	if (condition & PURPLE_GLIB_READ_COND)
		purple_cond =  PURPLE_INPUT_READ;
	if (condition & PURPLE_GLIB_WRITE_COND)
		purple_cond = PURPLE_INPUT_WRITE;

	closure->function(closure->data, g_io_channel_unix_get_fd(source),
			  purple_cond);

	return TRUE;
}

static guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
							   gpointer data)
{
	PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;

	closure->function = function;
	closure->data = data;

	if (condition & PURPLE_INPUT_READ)
		cond = (GIOCondition)PURPLE_GLIB_READ_COND;
	if (condition & PURPLE_INPUT_WRITE)
		cond = (GIOCondition)PURPLE_GLIB_WRITE_COND;

#if defined _WIN32 && !defined WINPIDGIN_USE_GLIB_IO_CHANNEL
	channel = wpurple_g_io_channel_win32_new_socket(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
					      purple_glib_io_invoke, closure, purple_glib_io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}


static void
write_conv(PurpleConversation *conv, const char *who, const char *alias,
			const char *message, PurpleMessageFlags flags, time_t mtime)
{
	const char *name;
	if (alias && *alias)
		name = alias;
	else if (who && *who)
		name = who;
	else
		name = NULL;

	printf("(%s) %s %s: %s\n", purple_conversation_get_name(conv),
			purple_utf8_strftime("(%H:%M:%S)", localtime(&mtime)),
			name, message);
}

static PurpleEventLoopUiOps glib_eventloops =
{
	g_timeout_add,
	g_source_remove,
	glib_input_add,
	g_source_remove,
	NULL,
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif

	/* padding */
	NULL,
	NULL,
	NULL
};

static PurpleConversationUiOps conv_uiops =
{
	NULL,                      /* create_conversation  */
	NULL,                      /* destroy_conversation */
	NULL,                      /* write_chat           */
	NULL,                      /* write_im             */
	write_conv,           /* write_conv           */
	NULL,                      /* chat_add_users       */
	NULL,                      /* chat_rename_user     */
	NULL,                      /* chat_remove_users    */
	NULL,                      /* chat_update_user     */
	NULL,                      /* present              */
	NULL,                      /* has_focus            */
	NULL,                      /* custom_smiley_add    */
	NULL,                      /* custom_smiley_write  */
	NULL,                      /* custom_smiley_close  */
	NULL,                      /* send_confirm         */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
ui_init(void)
{
	/**
	 * This should initialize the UI components for all the modules. Here we
	 * just initialize the UI for conversations.
	 */
	purple_conversations_set_ui_ops(&conv_uiops);
}

static PurpleCoreUiOps core_uiops =
{
	NULL,
	NULL,
	ui_init,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};



void purpleBindings::Initialize(v8::Handle<v8::Object> target)
{
	NanScope();

	v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(purpleBindings::New);
	t->InstanceTemplate()->SetInternalFieldCount(1);
	t->SetClassName(String::NewSymbol("purpleBindings"));
	
	
	 NODE_SET_PROTOTYPE_METHOD(t, "cc_purple_plugins_get_protocols", purpleBindings::cc_purple_plugins_get_protocols); //becomes global to be used by JS program
	 NODE_SET_PROTOTYPE_METHOD(t, "cc_purple_account_new", purpleBindings::cc_purple_account_new); //becomes global to be used by JS program
	 
	 
	 constructor = Persistent<Function>::New(t->GetFunction());
	 
	 /*Returns the unique function instance in the current execution context. */
	  target->Set(String::NewSymbol("purpleBindings"),/*t->GetFunction()*/constructor); 
	 
	 
    
}

NAN_METHOD(purpleBindings::cc_purple_plugins_get_protocols)
{
	NanScope();
	GList *iter;
	int i;
	GList *names = NULL;
	
/************************************Libpurple native related calls start******************************************************/

	v8::Local<v8::Array> pluginsArray = v8::Array::New(ARRAY_SIZE_FOR_PLUGIN);
	iter = purple_plugins_get_protocols();
	if(iter){
	for (i = 0; iter; iter = iter->next) {
		PurplePlugin *plugin = (PurplePlugin*)iter->data;
		PurplePluginInfo *info = plugin->info;
		if (info && info->name) {
			pluginsArray->Set(i, String::New(info->name));
			//printf("\t%d: %s\n", i++, info->name);
			names = g_list_append(names, info->id);
			i++;
		}
	}
	
/*******************************************end***************************************************/	
	NanReturnValue(pluginsArray);}
	else
		NanReturnValue(String::New("Empty"));
}


/*v8::Handle<v8::Value>*/ void purpleBindings::account_signed_on_cb(PurpleConnection *gc, gpointer null)
{
	NanScope();
	//PurpleAccount *account = purple_connection_get_account(gc);
	
	//printf("Account account_authorization_denied:\n");
//NanReturnValue(String::New("MMMMMM"));
	Local<Value> argv[] = 
	{
         Local<Value>::New(String::New("Returned From connect_to_signals1"))
	};
     callback->Call(Context::GetCurrent()->Global(), 1, argv);
}

static void
signing_on_cb(PurpleConnection *gc, void *data)
{
	//PurpleAccount *account = purple_connection_get_account(gc);
	//printf("Account connectedklkklkkl: %s %s\n", account->username, account->protocol_id);
	Local<Value> argv[] = 
	{
         Local<Value>::New(String::New("Returned From connect_to_signals2"))
	};
     callback->Call(Context::GetCurrent()->Global(), 1, argv);	
}



static void
account_authorization_denied_cb(PurpleConnection *gc, gpointer null)
{
	//printf("MMMMMMMMMMMMMMMMMMMMMMMMMM");
	Local<Value> argv[] = 
	{
         Local<Value>::New(String::New("Returned From connect_to_signals3"))
	};
     callback->Call(Context::GetCurrent()->Global(), 1, argv);	
}


static void
connection_error_cb(PurpleConnection *gc,
                    PurpleConnectionError err,
                    const gchar *desc,
                    void *data)
{
	/*const gchar *username =
		purple_account_get_username(purple_connection_get_account(gc));
	purple_debug_misc("signals test", "connection-error (%s, %u, %s)\n",
		username, err, desc);*/
		
	Local<Value> argv[] = 
	{
         Local<Value>::New(String::New("Returned From connect_to_signals4"))
	};
     callback->Call(Context::GetCurrent()->Global(), 1, argv);
}



void purpleBindings::connect_to_signals(const Arguments &args)
{
	static int handle;
	
	
	purple_signal_connect(purple_connections_get_handle(), "signed-on", &handle,
				PURPLE_CALLBACK(purpleBindings::account_signed_on_cb), NULL);
				
				
	purple_signal_connect(purple_accounts_get_handle(), "connection-error",
							&handle, PURPLE_CALLBACK(signing_on_cb), NULL);
	
	purple_signal_connect(purple_accounts_get_handle(), "account-authorization-denied",
							&handle, PURPLE_CALLBACK(account_authorization_denied_cb), NULL);
	
	purple_signal_connect(purple_accounts_get_handle(), "connection-error",
							&handle, PURPLE_CALLBACK(connection_error_cb), NULL);
	
				
	callback = Local<Function>::Cast(args[3]);
	
	Local<Value> argv[] = 
	{
         Local<Value>::New(String::New("Returned From connect_to_signals"))
	};
     callback->Call(Context::GetCurrent()->Global(), 1, argv);
} 


NAN_METHOD(purpleBindings::cc_purple_account_new)
{
	NanScope();

	PurpleAccount *account;
	PurpleSavedStatus *status;

	if(args.Length() != 4)
		{
			v8::ThrowException(v8::String::New("CC_ERROR Args are less then three"));
		}
	else
		{
		
		v8::String::Utf8Value protocol(args[0]);
		v8::String::Utf8Value uname(args[1]);
		v8::String::Utf8Value passwd(args[2]);
		
		/* Create the account */
		account = purple_account_new((const char *)*uname, (const char *)*protocol); //args 1,args 0
		
		/* Get the password for the account */
		//password = getpass("Password: "); //args 2
		purple_account_set_password(account, (const char *)*passwd);
		
		/* It's necessary to enable the account first. */
		purple_account_set_enabled(account, UI_ID, TRUE);
		
		/* Now, to connect the account(s), create a status and activate it. */
		status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
		purple_savedstatus_activate(status);
		//printf("%s ",(unsigned const char *)*protocol);
		//NanReturnValue(String::New((const char*)*protocol));
		purpleBindings::connect_to_signals(args);
		//NanReturnValue(String::New((const char*)*protocol));
		//MakeCallback(args.This(), "cb", 0, NULL);
			
		}
NanReturnValue(String::New("Return"));
}



/*Purple constructor*/
purpleBindings::purpleBindings(int val) {
}
/*Purple desconstructor*/
purpleBindings::~purpleBindings() {
}

NAN_METHOD(purpleBindings::New) {
    NanScope();
	//assert(args.IsConstructCall());
	//int io_threads = 1;
	//printf("Test");
	purpleBindings *pur = new purpleBindings(1);
    pur->Wrap(args.Holder());
    NanReturnValue(args.This());
	
  }

 static void
  Initialize(Handle<Object> target) {
    NanScope();
	
	purple_util_set_user_dir(CUSTOM_USER_DIRECTORY);
	/* For Release make TRUE AS FALSE. */
	purple_debug_set_enabled(FALSE);
	/* Set the core-uiops, which is used to
	 * 	- initialize the ui specific preferences.
	 * 	- initialize the debug ui.
	 * 	- initialize the ui components for all the modules.
	 * 	- uninitialize the ui components for all the modules when the core terminates.
	 */
	purple_core_set_ui_ops(&core_uiops);
	/* Set the uiops for the eventloop. If your client is glib-based, you can safely
	 * copy this verbatim. */
	purple_eventloop_set_ui_ops(&glib_eventloops);
		/* Set path to search for plugins. The core (libpurple) takes care of loading the
	 * core-plugins, which includes the protocol-plugins. So it is not essential to add
	 * any path here, but it might be desired, especially for ui-specific plugins. */
	purple_plugins_add_search_path(CUSTOM_PLUGIN_PATH);
	
		if (!purple_core_init(UI_ID)) {
		/* Initializing the core failed. Terminate. */
		fprintf(stderr,
				"libpurple initialization failed. Dumping core.\n"
				"Please report this!\n");
		abort();
	}

	/* Create and load the buddylist. */
	purple_set_blist(purple_blist_new());
	purple_blist_load();

	/* Load the preferences. */
	purple_prefs_load();

	/* Load the desired plugins. The client should save the list of loaded plugins in
	 * the preferences using purple_plugins_save_loaded(PLUGIN_SAVE_PREF) */
	purple_plugins_load_saved(PLUGIN_SAVE_PREF);

	/* Load the pounces. */
	purple_pounces_load();
	
	printf(" Libpurple is initialized\n");
	
	purpleBindings::Initialize(target);

	
	}


} // namespace zmq


// module

 void
init(Handle<Object> target) {

  purple::Initialize(target);
}

NODE_MODULE(purple, init) //purple==object
