#ifndef __SIFT_HPP__
#define __SIFT_HPP__

extern "C" {
#include <highgui.h>
}

#include <list>
#include <vector>
#include <map>
#include <string>

struct kd_node;
struct feature;

namespace SIFT
{

  const std::string& get_last_error();


  // Interface for reference counting.
  
  class SObject
  {
  public:
    SObject() {};
    virtual unsigned add_ref() = 0;
    virtual unsigned remove_ref() = 0;
    virtual unsigned ref_count() = 0;
  };

  // Implementation of reference counting.

  class SObjectImpl : public SObject
  {
  public:
    SObjectImpl();
    
  protected:
    virtual ~SObjectImpl();
    
  public:
    virtual unsigned add_ref();
    virtual unsigned remove_ref();
    virtual unsigned ref_count();
    
  protected:
    volatile unsigned m_ref_count;
  };
  
  
  
  struct MatchResult
  {
    std::string label;
    float score;
    float percentage;
    
    MatchResult(const std::string& label, float score, float percentage);
    MatchResult(const MatchResult&);
    MatchResult& operator=(const MatchResult&);
    bool operator==(const MatchResult&) const;
  };
  
  typedef std::vector<MatchResult> MatchResultVector;
  typedef std::map<std::string,MatchResult> MatchResultMap;
  
  struct FeatureInfo
  {
    struct feature *features;
    int num_features;
    unsigned id;
  };
  
  typedef std::vector<FeatureInfo> FeatureInfoVector;
  
  /*
    Holds the features for all the images in the candidate set.
  */
  class Database : public SObjectImpl
  {
  public:
    Database();
    ~Database();
    
    unsigned const image_count();
    unsigned const feature_count();
    bool add_image(IplImage *image, const std::string& label);
    bool add_image_features(struct feature *features, int n, const std::string& label);
    bool add_image_file(const std::string& path, const std::string& label);
    bool remove_image(const std::string& label);
    MatchResultVector match(struct feature* features, int n, int max_nn_chks=200, double max_dist=50000.0);
    MatchResultVector exhaustive_match(struct feature* features, int n, int max_nn_chks=200, double dist_sq_ratio=0.49);
    std::list<std::string> all_labels();
    bool const contains_label(const std::string& label);
    const FeatureInfo* get_image_info(const std::string& label);
    
    bool save(const char *path, bool binary=true);
    bool save(bool binary=true);
    bool load(const char *path, bool binary=true);
    
    void coalesce();
    
  protected:
    const std::string& label_for_id(unsigned id);
    bool id_for_label(const std::string& label, unsigned *id);
    const std::string& feature_label(const struct feature *feature);
  protected:
    FeatureInfoVector m_uncoalesced_features;
    
    struct feature *m_coalesced_features;
    struct kd_node *m_kd_tree;
    bool m_need_to_coalesce;
    std::string m_filename;
    
    unsigned m_id_counter;
    unsigned m_num_features;
    std::map<unsigned, std::string> m_id_label_map;
    std::map<std::string, unsigned> m_label_id_map;
  };
  
}

#endif
