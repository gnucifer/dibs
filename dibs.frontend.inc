<?php
/**
 * @file
 * Front end functions for the DIBS module
 */

//@todo: fix mixing of camel-case and lowercase in dibs parameters

/**
 * Page with hidden form fields that redirects to the DIBS payment
 *
 * @param array $transaction
 *
 * @return string
 */
function dibs_frontend_redirect_page($transaction) {
  $transaction['payment_price'] = (float) $transaction['payment_price'];

  // Making some checks to check if the transaction is valid
  if (!empty($transaction['api_module']) && !empty($transaction['api_delta'])) {
    $info = module_invoke($transaction['api_module'], 'dibsapi', 'info', $transaction['api_delta']);
    if (empty($info) || !is_array($info)) {
      drupal_set_message(t('Error redirecting to DIBS: api_module and/or api_delta is not valid!'), 'error');
      return '';
    }
  }
  elseif (!empty($transaction['api_module']) || !empty($transaction['api_delta'])) {
    drupal_set_message(t('Error redirecting to DIBS: api_module or api_delta is missing!'), 'error');
    return '';
  }
  elseif ($transaction['payment_status'] > 0) {
    drupal_set_message(t('Error redirecting to DIBS: the payment was already made!'), 'error');
    return '';
  }

  // Loading settings
  $settings = dibs_load_settings($transaction['api_module'], $transaction['api_delta']);

  if (empty($settings)) {
    drupal_set_message(t('DIBS module is not configured for module %module, delta %delta.', array('%module' => $transaction['api_module'], '%delta' => $transaction['api_delta'])), 'error');
    return t('Redirect could not be performed due to missing configuration settings.');
  }
  $settings['auto_submit'] = 1;

  // Getting redirect form
  $form = drupal_get_form('dibs_frontend_redirect_form', $settings, $transaction);

  // We are forcing the accept-charset to be ISO-8859-1, else will the order and delivery
  // info sent to DIBS be shown with wrong characters in the payment window and in their
  // administration system.
  /*
  if ($settings['general']['type'] === DIBS_PAYMENT_WINDOW) {
    $form['#attributes']['accept-charset'] = 'ISO-8859-1';
  }
  */

  return theme('dibs_redirect_page', array('form' => $form, 'settings' => $settings, 'transaction' => $transaction));
}

function dibs_payment_data($settings, $transaction, $extra_params = array()) {
  $data = array();
  if (isset($transaction['payment_retry_count']) && $transaction['payment_retry_count']) {
    $data['payment_order_id'] =  $transaction['payment_order_id'] . '-' . $transaction['payment_retry_count'];
  }
  // Required data
  $data['merchant'] = $settings['general']['merchant'];

  $split = !empty($transaction['params']['split_payment_price']) ? count($transaction['params']['split_payment_price']) : 1;
  
  // Handle split payments.
  if ($split > 1) {
    $data['split'] = $split;
    foreach (array_values($transaction['params']['split_payment_price']) as $i => $payment_price) {
      $data['amount' . $i + 1] = $payment_price;
    }
  }
  else {
    $data['amount'] = $transaction['payment_price'];
  }

  $data['currency'] = !empty($transaction['payment_currency']) ? $transaction['payment_currency'] : $transaction['general']['currency'];
  $data['orderid'] = $transaction['payment_order_id'];

  $data['callbackurl'] = url($settings['callbacks']['callbackok'] . '/' . $transaction['api_hash'], array('absolute' => TRUE));
 
  // Optional fields
  if ($settings['general']['test_mode']) {
    $data['test'] = 1;
  }
  if ($settings['general']['account']) {
    $data['account'] = $settings['general']['account'];
  }
  if ($settings['advanced']['capturenow']) {
    $data['capturenow'] = 1;
  }
  if ($settings['advanced']['calcfee'] && $split === 1) {
    $data['calcfee'] = 1;
  }

  $accepturl = url($settings['callbacks']['accepturl'] . '/' . $transaction['api_hash'], array('absolute' => TRUE));
  $cancelurl = url($settings['callbacks']['cancelurl'] . '/' . $transaction['api_hash'], array('absolute' => TRUE));
  $lang = $settings['general']['lang']; //@todo: fix!
  
  // Payment Window fields
  if ($settings['general']['type'] === DIBS_PAYMENT_WINDOW) {
    if ($settings['paymentwindow']['color']) {
      $data['color'] = $settings['paymentwindow']['color'];
    }
    //This is just so stupid, dibs what are you thinking
    $data['acceptreturnurl'] = $accepturl;
    $data['cancelreturnurl'] = $cancelurl;
    $data['language'] = $lang;

    if(!empty($settings['paymentwindow']['paytype'])) {
      $data['payType'] = $settings['paymentwindow']['paytype'];
    }
  }
  // Flex Window fields
  elseif ($settings['general']['type'] === DIBS_FLEXWIN) {
    $data['accepturl'] = $accepturl;
    $data['cancelurl'] = $cancelurl;
    $data['lang'] = $lang;
    //TODO: why is this not available for payment window?
    $data['uniqueoid'] = 1;

    if ($settings['flexwindow']['color']) {
      $data['color'] = $settings['flexwindow']['color'];
    }
    if ($settings['flexwindow']['decorator'] && $settings['flexwindow']['decorator'] != 'custom') {
      $data['decorator'] = $settings['flexwindow']['decorator'];
    }
    else {
      $data['decorator'] = '';
    }
    if ($settings['flexwindow']['voucher']) {
      $data['voucher'] = $settings['flexwindow']['voucher'] ? 'yes' : '';
    }
    //TODO: According to documentation only flexwin seems to support order information?

    // Order and payment info fields
    // If order_info_long contains data are we not sending order_info_short to DIBS.
    // The reason for this is a bug in the DIBS administration. If both are sent,
    // will only the short version show up in the administration.

    //TODO: convert to payment window format (if possible)
    //billingMail etc
    if (!empty($transaction['order_info_long'])) {
      $data += dibs_create_order_line_data($transaction['order_info_long']);
    }
    else if (!empty($transaction['order_info_short'])) {
      $data['ordertext'] = $transaction['order_info_short'];
    }
    if (!empty($transaction['order_price_info'])) {
      $data += dibs_create_price_line_data($transaction['order_price_info']);
    }

    // Delivery line data
    foreach(array_filter(array_keys($transaction), 'dibs_delivery_line_key') as $key) {
      //Strip 'customer_'
      $lines[substr($key, 9)] = $transaction[$key];
    }

    $lines = array_filter($lines);

    if(!empty($lines)) {
      $data += dibs_create_delivery_line_data($lines);
    }

    if(!empty($settings['flexwindow']['paytype'])) {
      $data['payType'] = $settings['flexwindow']['payType'];
    }
  }

  $data = $extra_params + $data;
 
  //Allow other modules to make alterations
  drupal_alter('dibs_payment_data', $data, $transaction, $settings);

  //Calculate MAC, after this we should not allow anyone to alter the data, so form_alter is problematic
  //TODO: flexwin still uses md5? Seems so
  if ($settings['general']['type'] === DIBS_PAYMENT_WINDOW) {
    if ($settings['general']['hmac']) {
      ksort($data);
      $data['MAC'] = dibs_hmac(dibs_hash_string($data), $settings['general']['hmac_key']);
    }
  }
  else if ($settings['general']['type'] === DIBS_FLEXWIN) {
    if(!empty($settings['general']['md5'])) {
      $params = array();
      foreach(array('merchant', 'orderid', 'currency', 'amount') as $key) {
        $params[$key] = $data[$key];
      }
      $md5_key = md5($settings['general']['md5_key2'] . md5($settings['general']['md5_key1'] . dibs_hash_string($params)));
      $data['md5key'] = $md5_key;
    }
  }
  return $data;
}


function dibs_hash_string($params) {
  $hash_data = array();
  foreach($params as $key => $value) {
    $hash_data[] = $key . '=' . $value;
  }
  return implode('&', $hash_data);
}

function dibs_delivery_line_key($key) {
  return strpos($key, 'customer_') === 0;
}

function dibs_transaction_key($key) {
  return preg_match('/^transact[0-9]*$/', $key);
}


/**
 * Form builder; Building the payment form and submits
 * it to DIBS with javascript
 *
 * @param array $form_state
 * @param array $settings
 * @param array $data
 *
 * @return array
 */
//@todo: The $extra_params is a temporary hack, fix properly later at more thorough refactor of dibs-api
function dibs_frontend_redirect_form($form, &$form_state = array(), $settings, $transaction, $extra_params = array()) {
  
  $data = dibs_payment_data($settings, $transaction, $extra_params);

  dsm('data');
  dsm($data);

  foreach($data as $key => $value) {
    $form[$key] = array(
      '#type' => 'hidden',
      '#value' => $value,
    );
  }

  //TODO: fix empty value in admin form
  // Payment Window fields
  if ($settings['general']['type'] === DIBS_PAYMENT_WINDOW) {
    //$form['#action'] = 'https://payment.architrade.com/payment/start.pml';
    $form['#action'] = DIBS_PAYMENT_WINDOW_URL;
  }
  // Flex Window fields
  elseif ($settings['general']['type'] === DIBS_FLEXWIN) {
    //$form['#action'] = 'https://payment.architrade.com/paymentweb/start.action';
    $form['#action'] = DIBS_FLEXWIN_URL;
  }

  // Additional form fields
  $form['submit'] = array('#type' => 'submit', '#value' => t('Continue to DIBS payment'));
  if (!empty($settings['auto_submit']) && FALSE) {
    $form['submit']['#attached']['js'][] = drupal_get_path('module', 'dibs') . '/dibs.auto_submit.js';
  }

  // Executing the hook_dibsapi_form_alter
  //TODO: why not use drupals standard form alter facilities?
  //Replaced by hook_dibs_payment_data_alter
  /*
  $func = $transaction['api_module'] .'_dibsapi_form_alter';
  if (function_exists($func)) {
    $func($form, $data['api_delta']);
  }
  */
  return $form;
}


/**
 * The cancel page where the user has the possibility to go back to the
 * payment again
 *
 * @param array $transaction
 *
 * @return string
 */

//TODO: save canceled status, hash-signature validation?
function dibs_frontend_cancel_page($transaction) {
  $transaction['payment_price'] = (float)$transaction['payment_price'];

  // Making some checks to check if the transaction is valid
  if (!empty($transaction['api_module']) && !empty($transaction['api_delta'])) {
    $info = module_invoke($transaction['api_module'], 'dibsapi', 'info', $transaction['api_delta']);
    if (empty($info) || !is_array($info)) {
      drupal_set_message(t('Error redirecting to DIBS: api_module and/or api_delta is not valid!'), 'error');
      return '';
    }
  }
  elseif (!empty($transaction['api_module']) || !empty($transaction['api_delta'])) {
    drupal_set_message(t('Error redirecting to DIBS: api_module or api_delta is missing!'), 'error');
    return '';
  }
  elseif ($transaction['payment_status'] > 0) {
    drupal_set_message(t('Error redirecting to DIBS: the payment has already made!'), 'error');
    return '';
  }

  // Invoking the module that implemented the payment
  $function = $transaction['api_module'] . '_dibsapi';
  if (function_exists($function)) {
    $function('transaction_cancel', $transaction['api_delta'], $transaction);
  }

  // Loading settings
  $settings = dibs_load_settings($transaction['api_module'], $transaction['api_delta']);

  // Getting redirect form
  $form = drupal_get_form('dibs_frontend_cancel_form', $settings, $transaction);

  return theme('dibs_cancel_page', array('form' => $form, 'settings' => $settings, 'transaction' => $transaction));
}

/**
 * Form builder; Building the cancel form
 *
 * @param array $form_state
 * @param array $settings
 * @param array $data
 *
 * @return array
 *
 * @ingroup forms
 * @see dibs_frontend_cancel_form_submit()
 */
function dibs_frontend_cancel_form($node, &$form_state = array(), $settings, $data) {
  $form['hash'] = array('#type' => 'hidden', '#value' => $data['api_hash']);
  $form['submit'] = array('#type' => 'submit', '#value' => t('Return to DIBS payment'));
  return $form;
}

/**
 * Process the cancel form
 *
 * @param array $form
 * @param array $form_state
 *
 * @ingroup forms
 * @see dibs_frontend_cancel_form()
 */
function dibs_frontend_cancel_form_submit($form, &$form_state) {
  $hash = $form_state['values']['hash'];

  // Loading the transaction
  $data = dibs_load_transaction($hash, 'hash');
  $settings = dibs_load_settings($data['api_module'], $data['api_delta'], TRUE);

  // Make changes to order id if dictated by retry policy
  $retry = isset($settings['general']['retry_handling']) ? $settings['general']['retry_handling'] : 'new_order_id';
  switch ($retry) {
    case 'new_order_id':
      // Generating order id
      $order_id = dibs_get_next_order_id($data['api_module'], $data['api_delta']);
      // Updating transaction
      $updated = db_update('dibs_transactions')
        ->fields(array(
          'payment_order_id' => $order_id,
          ))
        ->condition('api_hash', $hash)
        ->execute();
      break;
    case 'add_retry_suffix':
      $suffixed = array(
        'api_hash' => $hash,
        'payment_retry_count' => $data['payment_retry_count']+1,
      );
      drupal_write_record('dibs_transactions', $suffixed, array('api_hash'));
  }

  // Redirecting to the payment redirect page
  drupal_goto('payment/dibs/redirect/' . $hash);

  exit;
}

/**
 * Utility function to check if the $_REQUEST array has the needed transact
 * keys to perform validation.
 *
 * @param $request
 *    The $_REQUEST array
 * @param $split
 *    The number of payments the order is split into.
 *
 * @return boolean if the request is valid.
 */
//TODO: This feels redundant
function dibs_frontend_validate_transact_split($request, $split = 1) {
  if ($split == 1) {
    return isset($request['transact']);
  }
  foreach (range(1, $split) as $key) {
    if (!isset($request['transact' . $key])) {
      return FALSE;
    }
  }
  return TRUE;
}

function dibs_extract_transactions($data) {
  $transactions = array();
  foreach(array_filter(array_keys($data), 'dibs_transaction_key') as $key) {
    $transactions[$key] = $data[$key];
  }
  //Sort just to be safe
  ksort($transactions);
  return $transactions;
}

/**
 * DIBS payment ok callback
 *
 * @param array $transaction
 */
//Also transaction parameter is transact for flexwin and transaction for payment window
function dibs_frontend_dibs_callback_ok($transaction) {
  if(empty($transaction)) {
    watchdog('dibs', 'Invalid transaction hash in transaction callback', array(), WATCHDOG_ERROR);
    return MENU_ACCESS_DENIED;
  }
  try {
    $transaction = dibs_handle_response($_POST, $transaction, 'transaction_callback', FALSE);
  }
  catch (Exception $e) {
    watchdog_exception('dibs', $e);
    return MENU_ACCESS_DENIED;
  }
}

/**
 * DIBS accept page
 *
 * @param array $transaction
 */
function dibs_frontend_accept_page($transaction) {
  // Returning an error if the transaction is emtpy
  if (empty($transaction)) {
    drupal_set_message(t('An error occured. The transaction hash is not valid!'), 'error');
    watchdog('dibs', 'Invalid transaction hash on accept page', array(), WATCHDOG_ERROR);
    return MENU_ACCESS_DENIED;
  }
  // Unsetting the temporary session
  unset($_SESSION['dibs']);
  try {
    $transaction = dibs_handle_response($_POST, $transaction, 'transaction_accept');
  }
  catch (Exception $e) {
    return MENU_ACCESS_DENIED;
  }
  // Returning the themed output
  $settings = dibs_load_settings($transaction['api_module'], $transaction['api_delta']);
  return theme('dibs_accept_page', array('settings' => $settings, 'transaction' => $transaction));
}

//Handle response to accept and callback-url for payment window and flexwin
function dibs_handle_response($response, $transaction, $op, $messages = TRUE) {
  $settings = dibs_load_settings($transaction['api_module'], $transaction['api_delta']);
  try {
    switch($settings['general']['type']) {
      case DIBS_PAYMENT_WINDOW:
        $transaction = dibs_handle_payment_window_response($response, $transaction, $settings, $messages);
        break;
      case DIBS_FLEXWIN:
        $transaction = dibs_handle_flexwin_response($response, $transaction, $settings, $messages);
        break;
      default:
        throw new Exception(
          t(
            'Invalid payment window type in settings for @api_module:$api_delta',
            array('@api_module' => $transaction['api_module'], '@api_delta' => $transaction['api_delta'])
          )
        );
    }
  }
  catch (Exception $e) {
    watchdog_exception('dibs', $e);
    throw $e;
  }
  // Invoking the module that implemented the payment
  $function = $transaction['api_module'] . '_dibsapi';
  if (function_exists($function)) {
    $function($op, $transaction['api_delta'], $transaction);
  }
}

function dibs_handle_payment_window_response($response, $transaction, $settings, $messages = TRUE) {
  if(isset($response['transaction'])) {
    if (
      !(
        empty($settings['general']['hmac']) ||
        dibs_response_hmac_validate($response, $settings['general']['hmac_key'])
      )
    ) {
      if($messages) {
        drupal_set_message(t('Unable to validate transaction data'), 'error');
      }
      //TODO: Throw/Catch specific exception class, catch all is dangerous and will mask other errors
      throw new Exception(t('HMAC didn\'t match for transaction #!tid' , array('!tid' => $transaction['tid'])));
    }

    $params = array(
      'merchant' => $settings['general']['merchant'],
      'transact' => $response['transaction'],
    );
    if(!empty($settings['general']['account'])){
      $params['account'] = $settings['general']['account']; 
    }
    //Status we get from payment window is pretty fuzzy, get proper status instead
    $transaction_status = dibs_transaction_status($params);
    //TODO: should perhaps save all dibs response data (serialized) in log on each update
    // Updating transaction record with payment transaction id

    //TODO: set payment status according to payment_transaction_status
    $update = array(
      'payment_status' => 1,
      'payment_transaction_status' => $transaction_status,
      'payment_type' => isset($response['paytype']) ? $response['paytype'] : NULL,
      'payment_fee' => isset($response['fee']) ? $response['fee'] : NULL,
      'payment_transaction_id' => $response['transaction'],
    );
    //TODO: should be replaced by drupal_write_record or update-function
    $updated = db_update('dibs_transactions')
      ->fields($update)
      ->condition('tid', $transaction['tid'])
      ->execute();

    // Reload the transaction.
    $transaction = dibs_transaction_load($transaction['tid']);
    

    return $transaction;
  }
  else {
    throw new Exception(t('Transaction not set in response data for transaction #!tid.', array('!tid' => $transaction['tid'])));
  }
}

function dibs_handle_flexwin_response($response, $transaction, $settings, $messages = TRUE) {
  //How does this work for payment window?
  $split = $transaction['split_payments'];
  if (dibs_frontend_validate_transact_split($response, $split)) {
    if (
      !(
        empty($settings['general']['md5']) ||
        dibs_response_md5_authkey_validate(
          $response,
          $transaction,
          $settings['general']['md5_key2'],
          $settings['general']['md5_key1']
        )
      )
    ) {
      if($messages) {
        drupal_set_message(t('Unable to validate transaction data'), 'error');
      }
      throw new Exception(t('MD5 authkey didn\'t match for transaction #!tid' , array('!tid' => $transaction['tid'])));
    }
  }
  else {
    throw new Exception(t('Transaction not set in response data for transaction #!tid.', array('!tid' => $transaction['tid'])));
  }

  $transaction_status = dibs_flexwin_transaction_status_mapping($response['statuscode']);

  // Updating transaction record with payment transaction id
  $update = array(
    'payment_status' => 1,
    'payment_transaction_status' => $transaction_status,
    'payment_type' => isset($response['paytype']) ? $response['paytype'] : NULL,
    'payment_fee' => isset($response['fee']) ? $response['fee'] : NULL,
  );
  if ($split == 1) {
    $update['payment_transaction_id'] = $response['transact'];
  }
  //TODO: should be replaced by drupal_write_record or update-function
  $updated = db_update('dibs_transactions')
    ->fields($update)
    ->condition('tid', $transaction['tid'])
    ->execute();
  if ($split > 1) {
    $delta = 0;
    foreach(dibs_extract_transactions($response) as $payment_transaction_id) {
      db_insert('dibs_split_payments')
        ->fields(array(
          'tid' => $transaction['tid'],
          'payment_transaction_id' => $payment_transaction_id,
          'delta' => ++$delta,
        ))
        ->execute();
    }
  }

  if ($updated) {
    watchdog('dibs', 'Transaction #@tid validated as OK.', array(
      '@tid' => $transaction['tid'],
    ), WATCHDOG_INFO);
  }

  // Reload the transaction.
  $transaction = dibs_transaction_load($transaction['tid']);

  return $transaction;
}

function dibs_response_hmac_validate($response, $hmac_key) {
  $dibs_mac = $response['MAC'];
  unset($response['MAC']);
  ksort($response);
  return $dibs_mac === dibs_hmac(dibs_hash_string($response), $hmac_key);
}

function dibs_response_md5_authkey_validate($response, $transaction, $key1, $key2) {
  $hash_params = dibs_extract_transactions($response);
  $hash_params['amount'] = $transaction['payment_price'];
  $hash_params['currency'] = $transaction['payment_currency'];
  return $response['authkey'] === md5($key1 . md5($key2 . dibs_hash_string($hash_params)));
}

//TODO: is there a correct mapping?
/*
function dibs_payment_window_status_mapping($status) {
  switch($status) {
    case 'ACCEPTED': //Declined (will this ever be the case?)
      return DIBS_TRANSACTION_DECLINED;
      break;
    case 'DECLINED': //Authorization approved(?)
      return DIBS_TRANSACTION_DECLINED;
      break;
    case 'PENDING': //Capture completed
      return DIBS_TRANSACTION_CAPTURE_PENDING;
      break;
    case 'CANCELED': //Capture pending
      return DIBS_TRANSACTION_CAPTURE_PENDING;
    default:
      return DIBS_TRANSACTION_UNKNOWN_STATUS;
  }
}
*/
//TODO: perhaps no need for this mapping since status code seems to be the same, should be enough to validate
function dibs_flexwin_transaction_status_mapping($status) {
  switch($status) {
    case '1': //Declined (will this ever be the case?)
      return DIBS_TRANSACTION_DECLINED;
      break;
    case '2': //Authorization approved(?)
      return DIBS_TRANSACTION_AUTHORIZATION_APPROVED;
      break;
    case '5': //Capture completed
      return DIBS_TRANSACTION_CAPTURE_COMPLETED;
      break;
    case '12': //Capture pending
      return DIBS_TRANSACTION_CAPTURE_PENDING;
    default:
      return DIBS_TRANSACTION_UNKNOWN_STATUS;
  }
}
