<?php
/**
 * @file webform_confirm_email.admin.inc
 *
 * @author    Matthias Weiss <matthias@more-onion.com>
 * @copyright Copyright (c) 2013 copyright
 */

const WEBFORM_CONFIRM_EMAIL_STANDARD             = 0;
const WEBFORM_CONFIRM_EMAIL_CONFIRMATION_REQUEST = 1;
const WEBFORM_CONFIRM_EMAIL_CONFIRMATION         = 2;

/**
 * Implements hook_webform_component_delete().
 *
 * If a webform component is being deleted we check
 * if this component is an email address field and this
 * field is used as the email "To:" field the settings
 * in the webform_confirm_email table are deleted;
 * note that the yet not send confirmation mails are not
 * deleted
 *
 * @param $component
 *   The Webform component being deleted.
 */
function webform_confirm_email_webform_component_delete($component) {
  $nid = (int) $component['nid'];
  $cid = (int) $component['cid'];
  if (!$nid || !$cid) {
    return;
  }

  $results = db_query(
    'SELECT eid ' .
    '  FROM {webform_emails} ' .
    '    WHERE nid = :nid ' .
    '    AND email = :email ',
    array(
      ':nid'   => $nid,
      ':email' => $cid
    )
  );
  foreach ($results as $wfemail) {
    if (empty($wfemail->eid)) {
      continue;
    }
    $eid = (int) $wfemail->eid;
    db_delete('webform_confirm_email')
      ->condition('nid', $nid)
      ->condition('eid', $eid)
      ->execute();

    // the webform module should take care of the following but since it doesn't...
    db_delete('webform_emails')
      ->condition('nid', $nid)
      ->condition('eid', $eid)
      ->execute();
  }
}

/**
 * callback function set in hook_menu().
 *
 * It creates the email settings form for the confirmation request emails;
 * It is called for new emails to be set or for existing emails to be edited
 */
function webform_confirm_email_confirmation_request_email_edit($node, $email = array()) {

  $form_state = array(
    'build_info' => array(
      'args' => array(
        $node,
        $email,
      ),
    ),
    'submit_handlers' => array(
      'webform_email_edit_form_submit',
      '_webform_confirm_email_edit_confirmation_request_email_submit',
    ),
  );
  $form = drupal_build_form(
    'webform_email_edit_form',
    $form_state
  );


  $instructions = $form['template']['tokens']['#markup'];
  $search = '<p>' .
    t('You may use special tokens in this field that will be replaced with dynamic values.') .
    '</p>';

  $confirmation_tokens = array(
    t(
      '@confirm_url - URL that is emailed to a submitee that will confirm his/her email address.',
      array('@confirm_url' => '%confirm_url')
    )
  );

  $replace = $search . theme(
    'item_list',
    array(
      'items' => $confirmation_tokens,
      'title' => t('Email confirmation variables')
    )
  );

  $form['template']['tokens']['#markup'] = str_replace($search, $replace, $instructions);

  return drupal_render($form);
}

/**
 * submit handler for confirmation request emails edit form
 */
function _webform_confirm_email_edit_confirmation_request_email_submit($form, &$form_state) {

  if (   isset($form_state['values']['eid']) == TRUE
      && isset($form['node']['#value']->nid) == TRUE
      && empty($form['eid']['#value'])       == TRUE /*-> new email*/) {

    $obj['eid']        = $form_state['values']['eid'];
    $obj['nid']        = $form['node']['#value']->nid;
    $obj['email_type'] = WEBFORM_CONFIRM_EMAIL_CONFIRMATION_REQUEST;

    drupal_write_record(
      'webform_confirm_email',
      $obj
    );
  }
}

/**
 * callback function set in hook_menu().
 *
 * It creates the delete email form for confirmation request emails
 */
function webform_confirm_email_delete($node, $email = array()) {

  $form_state = array(
    'build_info' => array(
      'args' => array(
        $node,
        $email,
      ),
    ),
    'submit_handlers' => array(
      'webform_email_delete_form_submit',
      '_webform_confirm_email_delete_submit',
    ),
  );
  $form = drupal_build_form(
    'webform_email_delete_form',
    $form_state
  );

  return drupal_render($form);
}

/**
 * submit handler for deleting confirmation request emails
 */
function _webform_confirm_email_delete_submit($form, &$form_state) {

  $node  = $form_state['values']['node'];
  $email = $form_state['values']['email'];

  if (   isset($email['eid']) == TRUE
      && isset($node->nid) == TRUE) {
    db_query(
      'DELETE FROM {webform_confirm_email} ' .
      '  WHERE nid = :nid ' .
      '  AND   eid = :eid ',
      array(
        ':nid' => $node->nid,
        ':eid' => $email['eid']
      )
    );
  }
}

/**
 * callback function set in hook_menu().
 * It creates the email settings form for the confirmation request emails
 */
function webform_confirm_email_confirmation_email_edit($node, $email = array()) {

  $form_state = array(
    'build_info' => array(
      'args' => array(
        $node,
        $email,
      ),
    ),
    'submit_handlers' => array(
      'webform_email_edit_form_submit',
      '_webform_confirm_email_edit_confirmation_email_submit',
    ),
  );
  $form = drupal_build_form(
    'webform_email_edit_form',
    $form_state
  );

  $form['#theme'] = array('webform_email_edit_form');

  return drupal_render($form);
}

/**
 * submit handler for confirmation request emails edit form
 *
 * this generously borrows code from the webform_email_edit_form_submit
 * function of the webform module
 */
function _webform_confirm_email_edit_confirmation_email_submit($form, &$form_state) {

  if (   isset($form_state['values']['eid']) == TRUE
      && isset($form['node']['#value']->nid) == TRUE
      && empty($form['eid']['#value'])       == TRUE /*-> new email*/) {

    $obj['eid']        = $form_state['values']['eid'];
    $obj['nid']        = $form['node']['#value']->nid;
    $obj['email_type'] = WEBFORM_CONFIRM_EMAIL_CONFIRMATION;

    drupal_write_record(
      'webform_confirm_email',
      $obj
    );
  }
}

/**
 * helper function to make the validate and submit handlers of webform happy
 */
function webform_confirm_email_webform_emails_form_validate(&$form, &$form_state) {

  foreach(array('email_option', 'email_component') as $index) {
    if (isset($form_state['values'][$index]) == FALSE) {
      $form_state['values'][$index] = NULL;
    }
  }
}

/**
 * Implements hook_form_FORM_ID_alter()
 *
 * this alters the webform_emails_form form; it is split in 3 sections, 1 section
 * for standard emails, one for confirmation request emails and one for confirmation
 * emails
 */
function webform_confirm_email_form_webform_emails_form_alter(&$form, &$form_state) {
  // ******** standard emails ********
  unset($form['add']);

  $form['emails']['add_button'] = $form['add_button'];
  unset($form['add_button']);
  $form['emails']['add_button']['#value']  = t('Add standard email');
  $form['emails']['add_button']['#submit'] = array('webform_emails_form_submit');
  array_unshift($form['#validate'], 'webform_confirm_email_webform_emails_form_validate');

  // ******** confirmation request emails ********
  $query = db_query(
    'SELECT eid '.
    '  FROM {webform_confirm_email} ' .
    '    WHERE nid        = :nid ' .
    '    AND   email_type = :type' ,
    array(
      'nid'  => (int) $form['#node']->nid,
      'type' => WEBFORM_CONFIRM_EMAIL_CONFIRMATION_REQUEST,
    )
  );

  $form['confirmation_request'] = array();
  while ($eid = $query->fetchField()) {
    $form['confirmation_request'][$eid]['email']   = $form['emails'][$eid]['email'];
    $form['confirmation_request'][$eid]['subject'] = $form['emails'][$eid]['subject'];
    $form['confirmation_request'][$eid]['from']    = $form['emails'][$eid]['from'];

    unset($form['emails'][$eid]);
  }

  $form['confirmation_request']['add_button'] = array(
    '#type'   => 'submit',
    '#value'  => t('Add confirmation request mail'),
    '#submit' => array('webform_confirm_email_confirmation_request_email_add'),
    '#weight' => 45,
  );

  // ******** confirmation emails ********
  $query = db_query(
    'SELECT eid '.
    '  FROM {webform_confirm_email} ' .
    '    WHERE nid        = :nid ' .
    '    AND   email_type = :type ' ,
    array(
      'nid'  => (int) $form['#node']->nid,
      'type' => WEBFORM_CONFIRM_EMAIL_CONFIRMATION,
    )
  );

  $form['confirmation'] = array();

  while ($eid = $query->fetchField()) {
    $form['confirmation'][$eid]['email']   = $form['emails'][$eid]['email'];
    $form['confirmation'][$eid]['subject'] = $form['emails'][$eid]['subject'];
    $form['confirmation'][$eid]['from']    = $form['emails'][$eid]['from'];

    unset($form['emails'][$eid]);
  }

  $form['confirmation']['add_button'] = array(
    '#type'   => 'submit',
    '#value'  => t('Add confirmation mail'),
    '#submit' => array('webform_confirm_email_confirmation_email_add'),
    '#weight' => 45,
  );
}

/**
 * submit handler callback from the confirmation request email add button
 *
 * set a redirect to the path node/xx/webform/confirmation_request/new where
 * in hook_menu a page callback is defined: webform_confirm_email_confirmation_request_edit
 */
function webform_confirm_email_confirmation_request_email_add($form, &$form_state) {
  $form_state['redirect'] = array(
    'node/' . $form['#node']->nid . '/webform/confirmation_request/new',
  );
}

/**
 * submit handler callback from the confirmation email add button
 *
 * set a redirect to the path node/xx/webform/confirmation/new where
 * in hook_menu a page callback is defined: webform_confirm_email_confirmation_edit
 */
function webform_confirm_email_confirmation_email_add($form, &$form_state) {
  $form_state['redirect'] = array(
    'node/' . $form['#node']->nid . '/webform/confirmation/new',
  );
}

/**
 * implements hook_theme_registry_alter().
 *
 * insert 2 theme hooks for the 'webform_emails_form' and the 'webform_email_add_form'
 */
function webform_confirm_email_theme_registry_alter(&$theme_registry) {

  $theme_registry['webform_emails_form']['function']    = 'theme_webform_confirm_email_emails_form';
  $theme_registry['webform_email_add_form']['function'] = 'theme_webform_confirm_email_email_add_form';
}

/**
 * theme hook defined in hook_theme_registry_alter().
 *
 * includes the original theme hook for the 'webform_emails_form' and adds handling of
 * confirmation request emails and confirmation emails
 */
function theme_webform_confirm_email_emails_form($variables) {
  $form = &$variables['form'];
  $node = &$form['#node'];
  $header = array(
    t('E-mail to'),
    t('Subject'),
    t('From'),
    array(
      'data'    => t('Operations'),
      'colspan' => 2
    )
  );

  $output = '';
  foreach(array(
      'emails'               => 'Standard emails (always send)',
      'confirmation_request' => 'Confirmation request emails (always send)',
      'confirmation'         => 'Confirmation emails (only send when the confirmation URL is used)') as $email_type => $title) {
    $rows = array();

    $eids = element_children($form[$email_type]);
    if (array_search('add_button', $eids) !== FALSE) {
      unset($eids[array_search('add_button', $eids)]);
    }
    if (array_search('add', $eids) !== FALSE) {
      unset($eids[array_search('add', $eids)]);
    }
    if (count($eids) > 0) {
      foreach ($eids as $eid) {
        // Add each component to a table row.
        $rows[] = array(
          drupal_render($form[$email_type][$eid]['email']),
          drupal_render($form[$email_type][$eid]['subject']),
          drupal_render($form[$email_type][$eid]['from']),
          l(t('Edit'),   'node/' . $node->nid . '/webform/' . $email_type . '/' . $eid),
          l(t('Delete'), 'node/' . $node->nid . '/webform/' . $email_type . '/' . $eid . '/delete'),
        );
      }
    }
    else {
      switch($email_type) {
        case 'emails':
          $no_email_comment = t('Currently not sending standard e-mails, add an e-mail recipient below.');
          break;
        case 'confirmation_request':
          $no_email_comment = t('Currently not sending confirmation request e-mails, add an e-mail recipient below.');
          break;
        case 'confirmation':
          $no_email_comment = t('Currently not sending confirmation e-mails, add an e-mail recipient below.');
          break;
      }
      $rows[] = array(array('data' => $no_email_comment, 'colspan' => 5));
    }

    // Add a row containing form elements for a new item.
    $row_add_email = array(
      array(
        'colspan' => 3,
        'data'    => drupal_render($form[$email_type]['add'])
      ),
      array(
        'colspan' => 2,
        'data'    => drupal_render($form[$email_type]['add_button'])
      ),
    );
    $rows[] =array('data' => $row_add_email, 'class' => array('webform-add-form'));
    $output .= '<h2>' . $title . '</h2>';
    $output .= theme(
      'table',
      array(
        'header' => $header,
        'rows'   => $rows,
        'attributes' => array('id' => 'webform-' . $email_type),
      )
    );
  }

  $output .= drupal_render_children($form);

  return $output;
}

/**
 * theme hook defined in hook_theme_registry_alter().
 *
 * includes the original theme hook for the 'webform_email_add_form' and adds handling of
 * confirmation request emails and confirmation emails
 */
function theme_webform_confirm_email_email_add_form($form) {

  $email_type = $form['#array_parents'][0];

  // Add a default value to the custom e-mail textfield.
  $form[$email_type . '_custom']['#attributes']['rel']   = t('email@example.com');
  $form[$email_type . '_custom']['#attributes']['class'] = 'webform-set-active webform-default-value';
  $form[$email_type . '_option']['custom']['#title']     = $form[$email_type . '_option']['custom']['#title'] . ': ' . drupal_render($form[$email_type . '_custom']);

  // Render the component value.
  $form[$email_type . '_component']['#attributes']['class'] = 'webform-set-active';
  $form[$email_type . '_option']['component']['#title'] = $form[$email_type . '_option']['component']['#title'] . ': ' . drupal_render($form[$email_type . '_component']);

  // For spacing consistency, every option is wrapped in webform-container-inline.
  foreach (element_children($form[$email_type . '_option']) as $option) {
    $form[$email_type . '_option'][$option]['#prefix'] = '<div class="webform-container-inline">';
    $form[$email_type . '_option'][$option]['#suffix'] = '</div>';
  }

  return drupal_render($form);
}