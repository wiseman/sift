#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <algorithm>

#include "SIFT.hpp"

extern "C" {
#include "imgfeatures.h"
#include "kdtree.h"
#include "sift.h"

#include <cxcore.h>
#include <cv.h>
#include <highgui.h>
}

using namespace SIFT;


SObjectImpl::SObjectImpl()
  : m_ref_count(0)
{
  add_ref();
}

SObjectImpl::~SObjectImpl()
{
}

unsigned SObjectImpl::add_ref()
{
  return ++m_ref_count;
}

unsigned SObjectImpl::remove_ref()
{
  unsigned result = 0;
  if (m_ref_count > 0) {
    result = --m_ref_count;
    if (m_ref_count == 0) {
      delete this;
      return 0;
    }
  }
  return result;
}

unsigned SObjectImpl::ref_count()
{
  return m_ref_count;
}


static std::string last_error("");
static int last_errno(0);

int get_last_errno()
{
  return last_errno;
}

const std::string& SIFT::get_last_error()
{
  return last_error;
}

static void set_last_error(const std::string& message, int err)
{
  last_error = message;
  last_errno = err;
}






MatchResult::MatchResult(const std::string& l, float s, float p)
    : label(l), score(s), percentage(p)
{
}

MatchResult::MatchResult(const MatchResult& result)
    : label(result.label), score(result.score), percentage(result.percentage)
{
}

MatchResult& MatchResult::operator=(const MatchResult& result)
{
    if (this != &result) {
        label = result.label;
        score = result.score;
        percentage = result.percentage;
    }
    return *this;
}

bool MatchResult::operator==(const MatchResult& result) const
{
    return label == result.label
        && score == result.score
        && percentage == result.percentage;
}


Database::Database()
: m_coalesced_features(NULL), m_kd_tree(NULL),
  m_need_to_coalesce(true), m_filename(""), m_id_counter(0), m_num_features(0)
{
}

Database::~Database()
{
    if (m_kd_tree) {
        kdtree_release(m_kd_tree);
        m_kd_tree = NULL;
    }
}


/*
  Extracts features from an image and adds them to the db, indexed by ID.
*/
bool Database::add_image_file(const std::string& path, const std::string& label)
{
    IplImage *image = cvLoadImage(path.c_str(), 1);
    if (!image) {
      std::stringstream s;
      s << "Error loading image '" << path << "': " << strerror(errno);
      set_last_error(s.str(), errno);
      return false;
    }
    bool result = add_image(image, label);
    cvReleaseImage(&image);
    return result;
}


template <class key_t, class value_t>
void dump_map(const std::map<key_t, value_t>& map)
{
  std::cerr << "Map dump\n";
  for (typename std::map<key_t,value_t>::const_iterator i = map.begin();
       i != map.end();
       i++)
    {
      std::cerr << i->first << " -> " << i->second << "\n";
    }
}

bool const Database::contains_label(const std::string& label)
{
  std::map<std::string,unsigned>::const_iterator i = m_label_id_map.find(label);
  return i != m_label_id_map.end();
}

bool Database::id_for_label(const std::string& label, unsigned *id)
{
  std::map<std::string,unsigned>::const_iterator i = m_label_id_map.find(label);
  if (i == m_label_id_map.end()) {
    return false;
  } else {
    *id = i->second;
    return true;
  }
}

const FeatureInfo* Database::get_image_info(const std::string& label)
{
    unsigned id;
    if (!id_for_label(label, &id)) {
        return NULL;
    }
    
    for (FeatureInfoVector::const_iterator i = m_uncoalesced_features.begin();
         i != m_uncoalesced_features.end();
         i++) {
        if (i->id == id) {
            return &(*i);
        }
    }
    return NULL;
}

bool Database::add_image(IplImage *image, const std::string& label)
{
    struct feature *features;
    int num_features;

    num_features = sift_features(image, &features);
    return add_image_features(features, num_features, label);
}

/*
  Extracts features from an image and adds them to the db, indexed by ID.
*/
bool Database::add_image_features(struct feature *features, int n, const std::string& label)
{
  if (contains_label(label)) {
    set_last_error(std::string("Can't add image with duplicate label '") +
		   label + std::string("'"), 0);
    return false;
  }

  int i;
  unsigned id = m_id_counter++;
  
  m_label_id_map[label] = id;
  m_id_label_map[id] = label;
  
  FeatureInfo f;
  f.num_features = n;
  f.features = features;
  f.id = id;
  for (i = 0; i < f.num_features; i++) {
    f.features[i].feature_data = (void*) id;
  }
  
  m_uncoalesced_features.push_back(f);
  m_need_to_coalesce = true;
  return true;
}

void Database::coalesce()
{
    if (!m_need_to_coalesce) {
        return;
    }

    int total_num_features = 0;

    for (FeatureInfoVector::const_iterator i = m_uncoalesced_features.begin();
         i != m_uncoalesced_features.end();
         i++) {
        total_num_features += i->num_features;
    }
    m_num_features = total_num_features;

    m_coalesced_features = new struct feature[total_num_features];
    int k = 0;
    for (FeatureInfoVector::const_iterator i = m_uncoalesced_features.begin();
         i != m_uncoalesced_features.end();
         i++) {
        for (int j = 0; j < i->num_features; j++) {
            m_coalesced_features[k++] = i->features[j];
        }
    }
     
    if (m_kd_tree) {
        kdtree_release(m_kd_tree);
    }
    m_kd_tree = kdtree_build(m_coalesced_features, total_num_features);
    m_need_to_coalesce = false;
}

unsigned const Database::feature_count ()
{
  coalesce();
  return m_num_features;
}

const std::string& Database::label_for_id(unsigned id)
{
    return m_id_label_map[id];
}

const std::string& Database::feature_label(const struct feature *feature)
{
    return label_for_id((unsigned)feature->feature_data);
}


static bool result_score_cmp(const MatchResult& r1, const MatchResult& r2)
{
    return r1.score > r2.score;
}


MatchResultVector Database::exhaustive_match(struct feature *features, int n, int max_nn_chks, double dist_sq_ratio)
{
    struct feature **neighbors;
    int k;
    MatchResultMap result_map;
    struct kd_node *kd_tree;

    for (FeatureInfoVector::const_iterator i = m_uncoalesced_features.begin();
         i != m_uncoalesced_features.end();
         i++) {
        int num_matches = 0;
        kd_tree = kdtree_build(i->features, i->num_features);
        for (int j = 0; j < n; j++) {
            k = kdtree_bbf_knn(kd_tree, features + j, 2, &neighbors, max_nn_chks);
            if (k == 2) {
                double d0 = descr_dist_sq( features + j, neighbors[0] );
                double d1 = descr_dist_sq( features + j, neighbors[1] );
                if (d0 < d1 * dist_sq_ratio) {
                    num_matches++;
                }
            }
            free(neighbors);
        }
        kdtree_release(kd_tree);
        if (num_matches > 0) {
            result_map.insert(MatchResultMap::value_type(m_id_label_map[i->id],
							 MatchResult(m_id_label_map[i->id], num_matches,
								     (100.0 * num_matches) / n)));
        }
    }
    
    MatchResultVector sorted_results;
    for (MatchResultMap::const_iterator i = result_map.begin();
	 i != result_map.end();
	 i++) {
      sorted_results.push_back(i->second);
    }
    std::sort(sorted_results.begin(), sorted_results.end(), result_score_cmp);

    return sorted_results;
}

MatchResultVector Database::match(struct feature *features, int n, int max_nn_chks, double max_dist)
{
    coalesce();

    struct feature* feat;
    struct feature** nbrs;
    int i, k;
    MatchResultMap result_map;

    /* For each feature in the test set, find the nearest feature in the
       candidate set.  The image associated with that feature gets 1.0 added
       to its score. */
    for (i = 0; i < n; i++) {
        feat = features + i;
        k = kdtree_bbf_knn(m_kd_tree, feat, 2, &nbrs, max_nn_chks);
        if (k > 0) {
            if (max_dist <= 0.0 || descr_dist_sq(feat, nbrs[0]) < max_dist) {
                std::string l = feature_label(nbrs[0]);
                MatchResultMap::iterator r = result_map.find(l);
                if (r == result_map.end()) {
                    result_map.insert(MatchResultMap::value_type(l, MatchResult(l, 1.0, 0.0)));
                } else {
                    r->second.score += 1.0;
                }
            
                // The second-closest neighbor gets 0.2 added to its score.  Note that
                // this could conceivably push the percentage value we calculate later to
                // > 100.
                if (k > 1) {
                    if (max_dist <= 0.0 || descr_dist_sq(feat, nbrs[1]) < max_dist) {
                        std::string l = feature_label(nbrs[1]);
                        r = result_map.find(l);
                        if (r == result_map.end()) {
                            result_map.insert(MatchResultMap::value_type(l, MatchResult(l, 0.2, 0.0)));
                        } else {
                            r->second.score += 0.2;
                        }
                    }
                }
            }
        }
    }

    // Now go through and compute the percentages.
    for (MatchResultMap::iterator i = result_map.begin();
         i != result_map.end();
         i++) {
        i->second.percentage = 100.0 * i->second.score / n;
    }

    MatchResultVector sorted_results;
    for (MatchResultMap::const_iterator i = result_map.begin();
	 i != result_map.end();
	 i++) {
      sorted_results.push_back(i->second);
    }
    std::sort(sorted_results.begin(), sorted_results.end(), result_score_cmp);

    return sorted_results;
}


bool Database::remove_image(const std::string& label)
{
    unsigned id;
    bool found_label = false;
    for (std::map<unsigned,std::string>::iterator i = m_id_label_map.begin();
         i != m_id_label_map.end();
         i++) {
      if (i->second == label) {
	id = i->first;
	found_label = true;
	m_id_label_map.erase(i);

	std::map<std::string,unsigned>::iterator i = m_label_id_map.find(label);
	m_label_id_map.erase(i);

	break;
      }
    }

    if (found_label) {
        for (FeatureInfoVector::iterator i = m_uncoalesced_features.begin();
             i != m_uncoalesced_features.end();
             i++) {
            if (i->id == id) {
                free(i->features);
                m_uncoalesced_features.erase(i);
                m_need_to_coalesce = true;
                break;
            }
        }
    }
    return found_label;
}

std::list<std::string> Database::all_labels()
{
    std::list<std::string> labels;
    for (std::map<unsigned,std::string>::iterator i = m_id_label_map.begin();
         i != m_id_label_map.end();
         i++) {
        labels.push_back(i->second);
    }
    return labels;
}

unsigned const Database::image_count()
{
    return m_label_id_map.size();
}


bool Database::save(bool binary)
{
  if (m_filename == std::string("")) {
    return false;
  }
  return save(m_filename.c_str(), binary);
}

bool Database::save(const char* path, bool binary)
{
    FILE* file;

    if (!(file = fopen(path, "w" ))) {
        fprintf( stderr, "Warning: error opening %s, %s, line %d\n",
                 path, __FILE__, __LINE__ );
        return false;
    }

    fprintf(file, "%d\n", image_count());
    for (std::map<unsigned,std::string>::const_iterator i = m_id_label_map.begin();
         i != m_id_label_map.end();
         i++) {
        fprintf(file, "%d\n%s\n", i->first, i->second.c_str());
    }

    for (FeatureInfoVector::const_iterator i = m_uncoalesced_features.begin();
         i != m_uncoalesced_features.end();
         i++) {
        int result;
        if (binary) {
            result = export_features_binary_f(file, i->features, i->num_features);
        } else {
            result = export_features_text_f(file, i->features, i->num_features);
        }
        if (result != 0) {
            return false;
        }
    }
    if (fclose(file)) {
        perror("Error saving database: close:");
        return false;
    } else {
        return true;
    }
}

bool Database::load(const char* path, bool binary)
{
    FILE *file;
    int i;

    if (!(file = fopen(path, "r" ))) {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return false;
    }

    unsigned max_id = 0;
    int num_images;
    
    /* Read number of images. */
    if (fscanf(file, " %u ", &num_images) != 1) {
        perror("File read error: num_images");
        return false;
    }

    for (i = 0; i < num_images; i++) {
        unsigned id;
        char label[4096];
        
        // Read ID.
        if (fscanf(file, " %u ", &id) != 1) {
            perror("File read error: id");
            return false;
        }
        // Read label.
        if (!fgets(label, sizeof label, file)) {
            perror("File read error: label");
            return false;
        }
        // Trim newline.
        label[strlen(label) - 1] = '\0';
	std::string l(label);
	m_id_label_map[id] = l;
	m_label_id_map[l] = id;

        if (id > max_id) {
            max_id = id;
        }
    }

    for (int i = 0; i < num_images; i++) {
        struct feature *features;
        int n;
        if (binary) {
            n = import_features_binary_f(file, FEATURE_LOWE, &features);
        } else {
            n = import_features_text_f(file, FEATURE_LOWE, &features);
        }
        if (n < 0) return false;

        FeatureInfo f;
        f.num_features = n;
        f.features = features;
        f.id = (unsigned) f.features[0].feature_data;
        m_uncoalesced_features.push_back(f);
    }

    fclose(file);
    m_filename = std::string(path);
    m_id_counter = max_id + 1;
    m_need_to_coalesce = true;
    return true;
}

