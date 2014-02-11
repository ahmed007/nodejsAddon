
#ifndef BINDING_H
#define BINDING_H

#include <v8.h>
#include <glib.h>
#include <node.h>
#include <node_version.h>
#include <node_buffer.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <set>
#include <purple.h>
#include "nan.h"

namespace purple {
class purpleBindings:public node::ObjectWrap {

public:
static void Initialize(v8::Handle<v8::Object> target);
static void connect_to_signals(const v8::Arguments &args);
static void account_signed_on_cb(PurpleConnection *gc, gpointer null);


private:

 explicit purpleBindings(int val);
 ~purpleBindings();
 static NAN_METHOD(New);
 static NAN_METHOD(cc_purple_plugins_get_protocols);
 static NAN_METHOD(cc_purple_account_new);
 static v8::Persistent<v8::Function> constructor; //test
};
}
#endif
