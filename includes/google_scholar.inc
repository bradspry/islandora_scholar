<?php

/**
 * @file
 * Module used to embed meta tags in the HEAD for use in indexing in Google
 * Scholar and Zotero.
 */

/**
 * Creates meta tags to be placed in the head of the HTML.
 *
 * @param AbstractObject $object
 *   A AbstractObject.
 *
 * @return array
 *   Associative array where the key is the name of the tag and the value is
 *   the content.
 */
function islandora_scholar_create_meta_tags($object) {
  // Need at least title, full name of at least one author, publication year.
  if (!isset($object['MODS']) || !islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $object['MODS'])) {
    return FALSE;
  }
  else {
    $tags = array();
    $tags['citation_author'] = array();
    $mods = $object['MODS']->content;
    $mods_xml = new SimpleXMLElement($mods);
    $mods_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');

    $title_results = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_title', '//mods:mods[1]/mods:titleInfo/mods:title'));
    $title = (string) reset($title_results);
    $subtitle_results = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_title_sub_title', '//mods:mods[1]/mods:titleInfo/mods:subTitle'));
    $subtitle = (string) reset($subtitle_results);

    if (!empty($title)) {
      if (!empty($subtitle)) {
        $tags['citation_title'] = "{$title}: {$subtitle}";
      }
      else {
        $tags['citation_title'] = $title;
      }
    }
    else {
      return FALSE;
    }
    foreach ($mods_xml->xpath('mods:name') as $name_xml) {
      $name_parts = array();
      $name_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
      $roles = $name_xml->xpath('mods:role/mods:roleTerm');
      $role = strtolower((string) reset($roles));
      if ($role) {
        if ($role == 'author') {
          foreach ($name_xml->xpath('mods:namePart') as $name_part) {
            if ((string) $name_part != '') {
              // Strip periods off the end.
              $np = (string) rtrim($name_part, '.');
              if ($name_part['type'] == 'given') {
                $name_parts['first_name'] = (string) $name_part;
              }
              if ($name_part['type'] == 'family') {
                $name_parts['last_name'] = $np;
              }
              if (!isset($name_part['type'])) {
                $name_parts['no_type'] = $np;
              }
            }
          }
        }
        if (isset($name_parts['last_name']) && isset($name_parts['first_name'])) {
          $tags['citation_author'][] = $name_parts['last_name'] . ', ' . $name_parts['first_name'];
        }
        elseif (isset($name_parts['no_type'])) {
          $tags['citation_author'][] = $name_parts['no_type'];
        }
      }
    }
    if (count($tags['citation_author']) == 0) {
      return FALSE;
    }

    $origin_date = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_origin_date', '//mods:originInfo/mods:dateIssued'));
    $part_date = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_part_date', '//mods:part/mods:date'));
    $related_date = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_related_date', '//mods:relatedItem[@type="host"]//mods:date'));
    $created_date = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_created_date', '//mods:originInfo/mods:dateCreated'));

    if ($origin_date) {
      $date = (string) reset($origin_date);
    }
    elseif ($part_date) {
      $date = (string) reset($part_date);
    }
    elseif ($related_date) {
      $date = (string) reset($related_date);
    }
    elseif ($created_date) {
      $date = (string) reset($created_date);
    }
    else {
      return FALSE;
    }

    // Google requires dates in yy/mm/dd format or just the year. As dates suck
    // and we don't have a consistent structure of input we will just return the
    // year for now.

    if ($date) {
      $date_string = islandora_scholar_parse_date_foryear($date);
      if ($date_string) {
        $tags['citation_publication_date'] = $date_string;
      }
      else {
        return FALSE;
      }
    }

    $host_title = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_host_title', '//mods:relatedItem[@type="host"]//mods:title'));
    $genre = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_genre', '//mods:mods[1]/mods:genre'));

    $genre = strtolower((string) reset($genre));
    switch ($genre) {
      case 'book section':
      case 'book chapter':
        $host_tag = 'citation_book_title';
        break;

      case 'conference proceeding':
        $host_tag = 'citation_conference_title';
        break;

      default:
        $host_tag = 'citation_journal_title';
        break;
    }
    if ($host_title) {
      $tags[$host_tag] = (string) reset($host_title);
    }

    $volume = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_volume', '//mods:mods[1]/mods:part/mods:detail[@type="volume"]/mods:number'));
    if ($volume) {
      $tags['citation_volume'] = (string) reset($volume);
    }

    $issn = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_issn', '//mods:identifier[@type="issn"]'));
    if ($issn) {
      $tags['citation_issn'] = str_replace(
          array("&", "=", ",", ";"),
          array('', '', '', ''),
          (string) reset($issn));
    }

    $isbn = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_isbn', '//mods:identifier[@type="isbn"]'));
    if ($isbn) {
      $tags['citation_isbn'] = (string) reset($isbn);
    }

    $issue = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_issue', '//mods:mods[1]/mods:part/mods:detail[@type="issue"]/mods:number'));
    if ($issue) {
      $tags['citation_issue'] = (string) reset($issue);
    }

    $start_page = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_start_page', '//mods:extent[@unit="page"]/mods:start'));
    if ($start_page) {
      $tags['citation_firstpage'] = (string) reset($start_page);
    }

    $end_page = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_end_page', '//mods:extent[@unit="page"]/mods:end'));
    if ($end_page) {
      $tags['citation_lastpage'] = (string) reset($end_page);
    }

    $online_date = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_online_date', '//mods:recordInfo/mods:recordCreationDate'));
    if ($online_date) {
      $date_string = islandora_scholar_parse_date_foryear($online_date);
      if ($date_string) {
        $tags['citation_online_date'] = $date_string;
      }
    }

    $doi = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_doi', '//mods:identifier[@type="doi"]'));
    if ($doi) {
      $tags['citation_doi'] = (string) reset($doi);
    }

    $degree_grantor = $mods_xml->xpath(variable_get('islandora_scholar_xpaths_degree_grantor', '//mods:mods/mods:name[@type="corporate"][mods:role/mods:roleTerm = "Degree grantor"]/mods:namePart'));
    if ($degree_grantor) {
      $tags['citation_dissertation_institution'] = (string) reset($degree_grantor);
    }

    if ($object['PDF']) {
      $tags['citation_pdf_url'] = url("islandora/object/$object->id/datastream/PDF/view", array('absolute' => TRUE));
    }

    $tags['citation_abstract_html_url'] = url("islandora/object/$object->id/", array('absolute' => TRUE));
  }

  return $tags;
}

/**
 * Return the year portion of a date.
 *
 * @param string|array $date
 *   The date to parse.  If it is array we use the first element in the array
 *
 * @return null|string
 *   returns the year if the date was parsable and NULL otherwise
 */
function islandora_scholar_parse_date_foryear($date) {
  if (is_array($date)) {
    $date = (string) reset($date);
  }
  try {
    // Check if date is currently just a year.
    if (!(DateTime::createFromFormat('Y', $date))) {
      $date_obj = new DateTime($date);
      $date_string = date_format($date_obj, 'Y');
      return $date_string;
    }
    else {
      return $date;
    }
  }
  catch (Exception $e) {
    // TODO: should we log this?
    return NULL;
  }
}

/**
 * Adds the meta tags to the HEAD of the html document.
 *
 * @param array $tags
 *   An associate array containing the name => content of the meta tags.
 */
function islandora_scholar_embed_tags($tags) {
  $weight = 1000;
  if ($tags != FALSE) {
    foreach ($tags as $name => $content) {
      if (is_array($content)) {
        foreach ($content as $key => $nested_val) {
          if ($name == 'citation_author') {
            $weight++;
          }
          $element = array(
            '#tag' => 'meta',
            '#attributes' => array(
              'name' => $name,
              'content' => htmlspecialchars($nested_val),
            ),
            '#weight' => $weight,
          );
          drupal_add_html_head($element, $key . '_' . $nested_val);
        }
      }
      else {
        $element = array(
          '#tag' => 'meta',
          '#attributes' => array(
            'name' => $name,
            'content' => htmlspecialchars($content),
          ),
        );
        drupal_add_html_head($element, $name);
      }
    }
  }
}
