#pragma once


class JNIClassesBase {
    friend class JNIClass;
protected:
    JNIEnv* jni;
public:
    explicit JNIClassesBase(JNIEnv* jniEnv) : jni(jniEnv) {}
    void setJni(JNIEnv* jniEnv) {
        jni = jniEnv;
    }
};


class JNIClass {
    JNIClassesBase* const jniClasses;
protected:
    const jclass handle; // NOLINT(misc-misplaced-const)
    JNIEnv* getJNI() {
        return jniClasses->jni;
    }
public:
    JNIClass(JNIClassesBase* const jniClasses, const char* name) : jniClasses(jniClasses),
    handle((jclass) jniClasses->jni->NewGlobalRef(jniClasses->jni->FindClass(name))) {}
    ~JNIClass() {
        jniClasses->jni->DeleteGlobalRef(handle);
    }
    operator jclass() { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        return handle;
    }
};


#define JCLASS(CLASS_NAME, QUALIFIED_NAME, ...) \
struct CLASS_NAME : public JNIClass { \
    explicit CLASS_NAME(JNIClassesBase* const jniClasses) : JNIClass(jniClasses, QUALIFIED_NAME) {} \
    __VA_ARGS__ \
} CLASS_NAME{this};


#define JFIELD(NAME, JAVA_NAME, TYPE) \
jfieldID NAME = getJNI()->GetFieldID(handle, JAVA_NAME, TYPE);

#define JSTATICFIELD(NAME, JAVA_NAME, TYPE) \
jfieldID NAME = getJNI()->GetStaticFieldID(handle, JAVA_NAME, TYPE);


#define JMETHOD(NAME, JAVA_NAME, SIGNATURE) \
jmethodID NAME = getJNI()->GetMethodID(handle, JAVA_NAME, SIGNATURE);

#define JSTATICMETHOD(NAME, JAVA_NAME, SIGNATURE) \
jmethodID NAME = getJNI()->GetStaticMethodID(handle, JAVA_NAME, SIGNATURE);