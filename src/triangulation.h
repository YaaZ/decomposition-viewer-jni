#pragma once


#include "decomposition.h"


struct Triangulation {


    std::vector<glm::dvec2> vertices;
    std::vector<decomposition::PolygonTree> polygonTree;
    std::vector<glm::ivec3> triangles;


    explicit Triangulation(const std::vector<std::vector<glm::dvec2>>& polygons) {
        std::vector<std::vector<int>> polygonVertexIndices;
        for(const std::vector<glm::dvec2>& polygon : polygons) {
            vertices.reserve(vertices.size() + polygon.size());
            std::vector<int> indices;
            for (auto i : polygon) {
                indices.push_back(vertices.size());
                vertices.push_back(i);
            }
            polygonVertexIndices.push_back(std::move(indices));
        }
        polygonTree = decomposition::buildPolygonTrees(vertices,
                decomposition::decomposePolygonGraph(vertices,
                        decomposition::insertSteinerVerticesForPolygons(vertices, polygonVertexIndices)
                )
        );
        std::vector<decomposition::PolygonWithHolesTree> polygonWithHolesTree = decomposition::buildPolygonAreaTrees(
                std::vector<decomposition::PolygonTree>(polygonTree)
        );
        // Iterate roots only (do not triangulate overlapping areas more than once)
        for(const decomposition::PolygonWithHoles& polygon : polygonWithHolesTree) {
            std::vector<glm::ivec3> polygonTriangles = decomposition::triangulatePolygonWithHoles(vertices, polygon);
            triangles.reserve(triangles.size() + polygonTriangles.size());
            triangles.insert(triangles.end(), polygonTriangles.begin(), polygonTriangles.end());
        }
    }


};