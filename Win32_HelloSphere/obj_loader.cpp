#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32) || defined(linux)
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#endif

#include "obj_loader.h"

struct ObjVec3
{
    float x, y, z;
};

struct ObjFaceVertex
{
    int vIndex;
    int nIndex;
};

struct ObjTriangle
{
    ObjFaceVertex a, b, c;
};

static std::vector<ObjVec3> gObjVertices;
static std::vector<ObjVec3> gObjNormals;
static std::vector<ObjTriangle> gObjTriangles;

bool hasObjModel()
{
    return !gObjTriangles.empty();
}

bool parseFaceVertex(const std::string& token, ObjFaceVertex& out)
{
    std::stringstream ss(token);
    std::string part;
    std::vector<std::string> parts;

    while (std::getline(ss, part, '/'))
    {
        parts.push_back(part);
    }

    out.vIndex = -1;
    out.nIndex = -1;

    if (parts.size() >= 1 && !parts[0].empty())
        out.vIndex = std::stoi(parts[0]) - 1;

    // v/vt/vn 형식 지원
    if (parts.size() >= 3 && !parts[2].empty())
        out.nIndex = std::stoi(parts[2]) - 1;

    return (out.vIndex >= 0);
}

bool loadObjModel(const char* filename)
{
    gObjVertices.clear();
    gObjNormals.clear();
    gObjTriangles.clear();

    std::ifstream fin(filename);
    if (!fin.is_open())
        return false;

    std::string line;

    while (std::getline(fin, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            ObjVec3 v;
            iss >> v.x >> v.y >> v.z;
            gObjVertices.push_back(v);
        }
        else if (prefix == "vn")
        {
            ObjVec3 n;
            iss >> n.x >> n.y >> n.z;
            gObjNormals.push_back(n);
        }
        else if (prefix == "f")
        {
            std::vector<ObjFaceVertex> faceVerts;
            std::string token;

            while (iss >> token)
            {
                ObjFaceVertex fv;
                if (parseFaceVertex(token, fv))
                {
                    faceVerts.push_back(fv);
                }
            }

            if (faceVerts.size() == 3)
            {
                ObjTriangle tri;
                tri.a = faceVerts[0];
                tri.b = faceVerts[1];
                tri.c = faceVerts[2];
                gObjTriangles.push_back(tri);
            }
            else if (faceVerts.size() == 4)
            {
                ObjTriangle tri1, tri2;

                tri1.a = faceVerts[0];
                tri1.b = faceVerts[1];
                tri1.c = faceVerts[2];

                tri2.a = faceVerts[0];
                tri2.b = faceVerts[2];
                tri2.c = faceVerts[3];

                gObjTriangles.push_back(tri1);
                gObjTriangles.push_back(tri2);
            }
        }
    }

    return !gObjTriangles.empty();
}

void drawObjModel()
{
    glBegin(GL_TRIANGLES);

    for (size_t i = 0; i < gObjTriangles.size(); ++i)
    {
        const ObjTriangle& t = gObjTriangles[i];

        if (t.a.vIndex < 0 || t.a.vIndex >= (int)gObjVertices.size()) continue;
        if (t.b.vIndex < 0 || t.b.vIndex >= (int)gObjVertices.size()) continue;
        if (t.c.vIndex < 0 || t.c.vIndex >= (int)gObjVertices.size()) continue;

        const ObjVec3& v1 = gObjVertices[t.a.vIndex];
        const ObjVec3& v2 = gObjVertices[t.b.vIndex];
        const ObjVec3& v3 = gObjVertices[t.c.vIndex];

        if (t.a.nIndex >= 0 && t.a.nIndex < (int)gObjNormals.size())
        {
            const ObjVec3& n1 = gObjNormals[t.a.nIndex];
            glNormal3f(n1.x, n1.y, n1.z);
        }
        glVertex3f(v1.x, v1.y, v1.z);

        if (t.b.nIndex >= 0 && t.b.nIndex < (int)gObjNormals.size())
        {
            const ObjVec3& n2 = gObjNormals[t.b.nIndex];
            glNormal3f(n2.x, n2.y, n2.z);
        }
        glVertex3f(v2.x, v2.y, v2.z);

        if (t.c.nIndex >= 0 && t.c.nIndex < (int)gObjNormals.size())
        {
            const ObjVec3& n3 = gObjNormals[t.c.nIndex];
            glNormal3f(n3.x, n3.y, n3.z);
        }
        glVertex3f(v3.x, v3.y, v3.z);
    }

    glEnd();
}