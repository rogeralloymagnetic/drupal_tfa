<?php

/**
 * @file
 * Drush integration for TFA.
 */

/**
 * Implements hook_drush_command().
 */
function tfa_drush_command() {
  $items['tfa-user-login'] = array(
    'description' => 'Display a one time login link for the given user account (defaults to uid 1).',
    'aliases' => array('tuli'),
    'bootstrap' => DRUSH_BOOTSTRAP_NONE,
    'handle-remote-commands' => TRUE,
    'arguments' => array(
      'user' => 'An optional uid, user name, or email address for the user to log in as. Default is to log in as uid 1. The uid/name/mail options take priority if specified.',
      'path' => 'Optional path to redirect to after logging in.',
    ),
    'options' => array(
      'browser' => 'Optional value denotes which browser to use (defaults to operating system default). Set to 0 to suppress opening a browser.',
      'uid' => 'A uid to log in as.',
      'redirect-port' => 'A custom port for redirecting to (e.g. when running within a Vagrant environment)',
      'name' => 'A user name to log in as.',
      'mail' => 'A user mail address to log in as.',
    ),
    'examples' => array(
      'drush tfa-user-login ryan node/add/blog' => 'Displays and opens default web browser (if configured or detected) for a one-time login link for the user with the username ryan and redirect to the path node/add/blog.',
      'drush tfa-user-login --browser=firefox --mail=drush@example.org admin/settings/performance' => 'Open firefox web browser, login as the user with the e-mail address drush@example.org and redirect to the path admin/settings/performance.',
    ),
  );
  return $items;
}

/**
 * Displays a one time login link for the given user.
 */
function drush_tfa_user_login($user = '', $path = NULL) {
  $args = func_get_args();

  // Redispatch if called against a remote-host so a browser is started on the
  // the *local* machine.
  $alias = drush_get_context('DRUSH_TARGET_SITE_ALIAS');
  if (drush_sitealias_is_remote_site($alias)) {
    $return = drush_invoke_process($alias, 'tfa-user-login', $args, drush_redispatch_get_options(), array('integrate' => FALSE));
    if ($return['error_status']) {
      return drush_set_error('Unable to execute user login.');
    }
    else {
      $link = $return['object'];
    }
  }
  else {
    if (!drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL)) {
      // Fail gracefully if unable to bootstrap Drupal.
      // drush_bootstrap() has already logged an error.
      return FALSE;
    }

    if (drush_get_option('uid', FALSE) || drush_get_option('name', FALSE) || drush_get_option('mail', FALSE)) {
      // If we only have a single argument and one of the user options is passed,
      // then we assume the argument is the path to open.
      if (count($args) == 1) {
        $path = $args[0];
      }
    }

    // Try to load a user from provided options and arguments.
    if ($uid = drush_get_option('uid', FALSE)) {
      $account = user_load($uid);
    }
    elseif ($name = drush_get_option('name', FALSE)) {
      $account = user_load_by_name($name);
    }
    elseif ($mail = drush_get_option('mail', FALSE)) {
      $account = user_load_by_mail($mail);
    }
    elseif (is_numeric($user) && $account = user_load($user)) {
    }
    elseif (($user != "") && ($account = user_load_by_name($user))) {
    }
    elseif (($user != "") && ($account = user_load_by_mail($user))) {
    }
    elseif ($account = user_load(1)) {
    }
    else {
      return drush_set_error(dt('Could not find the specified user.'));
    }

    // Construct login link.
    $options = array('absolute' => TRUE);
    if ($path) {
      $options['destination'] = $path;
    }
    $link = tfa_onetime_link($account, 'login', $options);
    variable_set('tfa_latest_login_link', REQUEST_TIME);
  }
  $port = drush_get_option('redirect-port', FALSE);
  drush_start_browser($link, FALSE, $port);
  return $link;
}

/**
 * Informs the user about TFA's presence when the uli command is used.
 */
function drush_tfa_post_user_login() {
  drush_log(dt("Two-factor Authentication (TFA) is enabled. To skip the TFA process use the 'drush tfa-user-login' ('drush tuli') command instead."), 'warning');
}
