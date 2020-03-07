#include <jni.h>

#define JNI_VERSION JNI_VERSION_10

#include "jclass.h"

#include <cstdint>
#include <vector>

#include "triangulation.h"


struct JNIClasses : public JNIClassesBase {
    explicit JNIClasses(JNIEnv* jni) : JNIClassesBase(jni) {}

    JCLASS(List, "java/util/List",
           JMETHOD(size, "size", "()I")
                   JMETHOD(get, "get", "(I)Ljava/lang/Object;")
    )
    JCLASS(Point2D, "java/awt/geom/Point2D",
           JMETHOD(getX, "getX", "()D")
                   JMETHOD(getY, "getY", "()D")
    )
    JCLASS(Point2DDouble, "java/awt/geom/Point2D$Double",
           JMETHOD(init, "<init>", "(DD)V")
    )
    JCLASS(PolygonSet, "yaaz/decomposition/viewer/polygon/PolygonSet",
           JFIELD(polygons, "polygons", "Ljava/util/List;")
    )
    JCLASS(Polygon, "yaaz/decomposition/viewer/polygon/Polygon",
           JFIELD(vertices, "vertices", "Ljava/util/List;")
    )
    JCLASS(Triangulation, "yaaz/decomposition/viewer/polygon/Triangulation",
           JFIELD(address, "address", "J")
    )
    JCLASS(DecomposedPolygon, "yaaz/decomposition/viewer/polygon/Triangulation$DecomposedPolygon",
           JMETHOD(init, "<init>", "(I[I)V")
    )
    JCLASS(Triangle, "yaaz/decomposition/viewer/polygon/Triangulation$Triangle",
           JMETHOD(init, "<init>", "(III)V")
    )

}* JClass;



static std::vector<std::vector<glm::dvec2>> convertJavaPolygonList(JNIEnv* jni, jobject polygonSet, jfieldID field) {
    jobject polygonList = jni->GetObjectField(polygonSet, field);
    jint polygonListSize = jni->CallIntMethod(polygonList, JClass->List.size);

    std::vector<std::vector<glm::dvec2>> result;
    result.reserve(polygonListSize);
    for (jint i = 0; i < polygonListSize; i++) {
        jobject polygon = jni->CallObjectMethod(polygonList, JClass->List.get, i);
        jobject vertexList = jni->GetObjectField(polygon, JClass->Polygon.vertices);
        jint vertexListSize = jni->CallIntMethod(vertexList, JClass->List.size);

        std::vector<glm::dvec2> resultVertexList;
        resultVertexList.reserve(vertexListSize);
        for (int j = 0; j < vertexListSize; j++) {
            jobject point2D = jni->CallObjectMethod(vertexList, JClass->List.get, j);
            resultVertexList.emplace_back(
                    (double) jni->CallDoubleMethod(point2D, JClass->Point2D.getX),
                    (double) jni->CallDoubleMethod(point2D, JClass->Point2D.getY)
            );
        }
        result.push_back(std::move(resultVertexList));
    }
    return result;
}


static Triangulation* unwrapTriangulation(JNIEnv* jni, jobject javaTriangulationObject) {
    return (Triangulation*) jni->GetLongField(javaTriangulationObject, JClass->Triangulation.address);
}


static void addSubtreeOfDebugPolygonsToJavaArray(JNIEnv* jni, const decomposition::PolygonTree& subtree,
        jobjectArray& resultPolygons, int& addedPolygons) {
    jintArray vertexIndices = jni->NewIntArray(subtree.vertexIndices.size());
    jni->SetIntArrayRegion(vertexIndices, 0, subtree.vertexIndices.size(), (jint*) subtree.vertexIndices.data());
    jobject javaPolygon = jni->NewObject(JClass->DecomposedPolygon, JClass->DecomposedPolygon.init, (jint) subtree.netWinding, vertexIndices);
    jni->SetObjectArrayElement(resultPolygons, addedPolygons++, javaPolygon);
    for(const decomposition::PolygonTree& subtreeChild : subtree.childrenPolygons) {
        addSubtreeOfDebugPolygonsToJavaArray(jni, subtreeChild, resultPolygons, addedPolygons);
    }
}
static int countPolygonsInTree(const std::vector<decomposition::PolygonTree>& tree) {
    int count = tree.size();
    for(const decomposition::PolygonTree& subtree : tree) count += countPolygonsInTree(subtree.childrenPolygons);
    return count;
}



extern "C" {


jint JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* jni;
    vm->GetEnv((void**) &jni, JNI_VERSION);
    JClass = new JNIClasses(jni);
    return JNI_VERSION;
}


void JNI_OnUnload(JavaVM* vm, void*) {
    JNIEnv* jni;
    vm->GetEnv((void**) &jni, JNI_VERSION);
    JClass->setJni(jni);
    delete JClass;
}



/*
 * Class:     yaaz_decomposition_viewer_polygon_Triangulation
 * Method:    create
 * Signature: (Lyaaz/decomposition/viewer/polygon/PolygonSet;)Lyaaz/decomposition/viewer/polygon/Triangulation;
 */
JNIEXPORT jobject JNICALL Java_yaaz_decomposition_viewer_polygon_Triangulation_create
        (JNIEnv* jni, jclass clazz, jobject polygonSet) {
    auto polygons = convertJavaPolygonList(jni, polygonSet, JClass->PolygonSet.polygons);
    auto triangulation = new Triangulation(polygons);
    return jni->NewObject(clazz, jni->GetMethodID(clazz, "<init>", "(J)V"), (jlong) triangulation);
}



/*
 * Class:     yaaz_decomposition_viewer_polygon_Triangulation
 * Method:    cleanup
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_yaaz_decomposition_viewer_polygon_Triangulation_cleanup
        (JNIEnv*, jclass, jlong address) {
    auto* triangulation = (Triangulation*) address;
    delete triangulation;
}



/*
 * Class:     yaaz_decomposition_viewer_polygon_Triangulation
 * Method:    getAllVertices
 * Signature: ()[Ljava/awt/geom/Point2D;
 */
JNIEXPORT jobjectArray JNICALL Java_yaaz_decomposition_viewer_polygon_Triangulation_getAllVertices
        (JNIEnv* jni, jobject javaTriangulationObject) {
    Triangulation* triangulation = unwrapTriangulation(jni, javaTriangulationObject);
    jobjectArray resultVertices = jni->NewObjectArray(triangulation->vertices.size(), JClass->Point2D, nullptr);
    for (int i = 0; i < triangulation->vertices.size(); i++) {
        glm::dvec2 vertex = triangulation->vertices[i];
        jobject javaVertex = jni->NewObject(JClass->Point2DDouble, JClass->Point2DDouble.init, (jdouble) vertex.x, (jdouble) vertex.y);
        jni->SetObjectArrayElement(resultVertices, i, javaVertex);
    }
    return resultVertices;
}



/*
 * Class:     yaaz_decomposition_viewer_polygon_Triangulation
 * Method:    getDecomposedPolygons
 * Signature: ()[Lyaaz/decomposition/viewer/polygon/Triangulation/DecomposedPolygon;
 */
JNIEXPORT jobjectArray JNICALL Java_yaaz_decomposition_viewer_polygon_Triangulation_getDecomposedPolygons
        (JNIEnv* jni, jobject javaTriangulationObject) {
    Triangulation* triangulation = unwrapTriangulation(jni, javaTriangulationObject);
    jobjectArray resultPolygons = jni->NewObjectArray(countPolygonsInTree(triangulation->polygonTree), JClass->DecomposedPolygon, nullptr);
    int addedPolygons = 0;
    for (const decomposition::PolygonTree& polygon : triangulation->polygonTree) {
        addSubtreeOfDebugPolygonsToJavaArray(jni, polygon, resultPolygons, addedPolygons);
    }
    return resultPolygons;
}


/*
 * Class:     yaaz_decomposition_viewer_polygon_Triangulation
 * Method:    getTriangles
 * Signature: ()[Lyaaz/decomposition/viewer/polygon/Triangulation/Triangle;
 */
JNIEXPORT jobjectArray JNICALL Java_yaaz_decomposition_viewer_polygon_Triangulation_getTriangles
        (JNIEnv* jni, jobject javaTriangulationObject) {
    Triangulation* triangulation = unwrapTriangulation(jni, javaTriangulationObject);
    jobjectArray resultTriangles = jni->NewObjectArray(triangulation->triangles.size(), JClass->Triangle, nullptr);
    int addedTriangles = 0;
    for (const glm::ivec3& triangle : triangulation->triangles) {
        jobject javaTriangle = jni->NewObject(JClass->Triangle, JClass->Triangle.init, (jint) triangle.x, (jint) triangle.y, (jint) triangle.z);
        jni->SetObjectArrayElement(resultTriangles, addedTriangles++, javaTriangle);
    }
    return resultTriangles;
}


}
