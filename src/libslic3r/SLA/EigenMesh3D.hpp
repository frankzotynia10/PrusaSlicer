#ifndef SLA_EIGENMESH3D_H
#define SLA_EIGENMESH3D_H

#include <libslic3r/SLA/Common.hpp>

namespace Slic3r {

class TriangleMesh;

namespace sla {

struct Contour3D;

/// An index-triangle structure for libIGL functions. Also serves as an
/// alternative (raw) input format for the SLASupportTree.
//  Implemented in SLASupportTreeIGL.cpp
class EigenMesh3D {
    class AABBImpl;
    
    Eigen::MatrixXd m_V;
    Eigen::MatrixXi m_F;
    double m_ground_level = 0, m_gnd_offset = 0;
    
    std::unique_ptr<AABBImpl> m_aabb;
public:
    
    EigenMesh3D(const TriangleMesh&);
    EigenMesh3D(const EigenMesh3D& other);
    EigenMesh3D(const Contour3D &other);
    EigenMesh3D& operator=(const EigenMesh3D&);
    
    ~EigenMesh3D();
    
    inline double ground_level() const { return m_ground_level + m_gnd_offset; }
    inline void ground_level_offset(double o) { m_gnd_offset = o; }
    inline double ground_level_offset() const { return m_gnd_offset; }
    
    inline const Eigen::MatrixXd& V() const { return m_V; }
    inline const Eigen::MatrixXi& F() const { return m_F; }
    
    // Result of a raycast
    class hit_result {
        double m_t = std::nan("");
        int m_face_id = -1;
        const EigenMesh3D *m_mesh = nullptr;
        Vec3d m_dir;
        Vec3d m_source;
        friend class EigenMesh3D;
        
        // A valid object of this class can only be obtained from
        // EigenMesh3D::query_ray_hit method.
        explicit inline hit_result(const EigenMesh3D& em): m_mesh(&em) {}
    public:
        
        // This can create a placeholder object which is invalid (not created
        // by a query_ray_hit call) but the distance can be preset to
        // a specific value for distinguishing the placeholder.
        inline hit_result(double val = std::nan("")): m_t(val) {}
        
        inline double distance() const { return m_t; }
        inline const Vec3d& direction() const { return m_dir; }
        inline Vec3d position() const { return m_source + m_dir * m_t; }
        inline int face() const { return m_face_id; }
        inline bool is_valid() const { return m_mesh != nullptr; }
        
        // Hit_result can decay into a double as the hit distance.
        inline operator double() const { return distance(); }
        
        inline Vec3d normal() const {
            if(m_face_id < 0 || !is_valid()) return {};
            auto trindex    = m_mesh->m_F.row(m_face_id);
            const Vec3d& p1 = m_mesh->V().row(trindex(0));
            const Vec3d& p2 = m_mesh->V().row(trindex(1));
            const Vec3d& p3 = m_mesh->V().row(trindex(2));
            Eigen::Vector3d U = p2 - p1;
            Eigen::Vector3d V = p3 - p1;
            return U.cross(V).normalized();
        }
        
        inline bool is_inside() {
            return m_face_id >= 0 && normal().dot(m_dir) > 0;
        }
    };
    
    // Casting a ray on the mesh, returns the distance where the hit occures.
    hit_result query_ray_hit(const Vec3d &s, const Vec3d &dir) const;
    
    // Casts a ray on the mesh and returns all hits
    std::vector<hit_result> query_ray_hits(const Vec3d &s, const Vec3d &dir) const;
    
    class si_result {
        double m_value;
        int m_fidx;
        Vec3d m_p;
        si_result(double val, int i, const Vec3d& c):
            m_value(val), m_fidx(i), m_p(c) {}
        friend class EigenMesh3D;
    public:
        
        si_result() = delete;
        
        double value() const { return m_value; }
        operator double() const { return m_value; }
        const Vec3d& point_on_mesh() const { return m_p; }
        int F_idx() const { return m_fidx; }
    };

#ifdef SLIC3R_SLA_NEEDS_WINDTREE
    // The signed distance from a point to the mesh. Outputs the distance,
    // the index of the triangle and the closest point in mesh coordinate space.
    si_result signed_distance(const Vec3d& p) const;
    
    bool inside(const Vec3d& p) const;
#endif /* SLIC3R_SLA_NEEDS_WINDTREE */
    
    double squared_distance(const Vec3d& p, int& i, Vec3d& c) const;
    inline double squared_distance(const Vec3d &p) const
    {
        int   i;
        Vec3d c;
        return squared_distance(p, i, c);
    }
};

// Calculate the normals for the selected points (from 'points' set) on the
// mesh. This will call squared distance for each point.
PointSet normals(const PointSet& points,
    const EigenMesh3D& convert_mesh,
    double eps = 0.05,  // min distance from edges
    std::function<void()> throw_on_cancel = [](){},
    const std::vector<unsigned>& selected_points = {});

}} // namespace Slic3r::sla

#endif // EIGENMESH3D_H