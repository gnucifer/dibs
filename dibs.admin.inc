<?php

/**
 * @file
 * Admin functions for the DIBS module
 */

/**
 * List of implementations of the DIBS module.
 * Link to edit settings for each implementation
 * @return string
 */
function dibs_admin_settings_list() {
  drupal_add_css(drupal_get_path('module', 'dibs') .'/dibs.admin.css');

  // Looping through all modules that implement the hook_dibsapi.
  $attributes['class'][] = 'dibs-admin-settings-list';
  $output = '<br />';
  $i = 0;
  foreach ((array)module_implements('dibsapi') as $module) {
    if ($i++ > 0) {
      $output .= '<p></p>';
    }
    $output .= '<h3>' . t('Module') . ': ' . $module . '</h3>';

    // Adding table headers
    $header = array(
      array('data' => t('Name'), 'class' => 'dibs-admin-settings-title'),
      array('data' => t('Operation'), 'class' => 'dibs-admin-settings-operation')
    );

    $rows = array();
    // Looping through deltas for a specific module implementation of hook_dibsapi)).
    foreach ((array)module_invoke($module, 'dibsapi') as $delta => $info) {
      $row = array(
        array('data' => $info['info'], 'class' => ''),
        array('data' => l(t('edit'), 'admin/config/payment/dibs/edit/' . $module . '/' . $delta), 'class' => ''),
      );
      $rows[] = $row;
    }
    $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
  }

  return $output;
}

/**
 * Form builder; The settings edit form
 *
 * @param array $form_state
 * @param string $module
 * @param multi $delta
 *
 * @ingroup forms
 * @see dibs_admin_settings_form_validate()
 * @see dibs_admin_settings_form_submit()
 */
function dibs_admin_settings_form($form, &$form_state, $module = NULL, $delta = NULL) {
  // Checking if the module and delta exists
  $info = module_invoke($module, 'dibsapi', 'info', $delta);
  if (empty($info) || !is_array($info)) {
    drupal_not_found();
    exit;
  }

  // Checking if module and delta are default
  $default = dibs_is_default($module, $delta);

  // Setting page title
  drupal_set_title(t('Change DIBS implementation settings: @module > @delta', array('@module' => t($module), '@delta' => $info['info'])));

  // Loading settings
  $settings = dibs_load_settings($module, $delta, FALSE);

  // General
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General DIBS settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );
  $form['general']['merchant'] = array(
    '#type' => 'textfield',
    '#title' => t('MertchantID'),
    '#maxlength' => 30,
    '#required' => $default,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'merchant', ''),
    '#description' => t('DIBS MerchantID'),
  );
  $form['general']['account'] = array(
    '#type' => 'textfield',
    '#title' => t('Account'),
    '#maxlength' => 64,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'account', ''),
    '#description' => t('DIBS Account ID. Only used if the DIBS gateway is running multiple accounts.'),
  );
  $form['general']['test_mode'] = array(
    '#type' => 'select',
    '#title' => t('Test mode'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'test_mode', ''),
    '#description' => t('Is the gateway running in test mode'),
  );
  //TODO: fix rest of default values
  $options = $default ? array() : array('' => t('Default'));
  $options += array(DIBS_PAYMENT_WINDOW => t('Pay window'), DIBS_FLEXWIN => t('Flex window'));

  $form['general']['type'] = array(
    '#type' => 'select',
    '#title' => t('Window type'),
    '#required' => $default,
    '#options' => $options,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'type', $default ? DIBS_PAYMENT_WINDOW : ''),
    '#description' => t('If enabled, DIBS will make some extra checks on the sended data, to be sure that no one manipulated it. If enabled should the keys below be filled in!'),
  );

  $form['general']['retry_handling'] = array(
    '#type' => 'select',
    '#title' => t('Order id handling after cancel'),
    '#options' => array('' => '', 'new_order_id' => t('Generate a new order id'), 'add_retry_suffix' => t('Add retry suffix'), 'unchanged' => t('Preserve order id')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'retry_handling', ($default ? 'new_order_id' : '')),
    '#description' => t('How the order id should be handled when the user retries a cancelled payment. Some card providers (edankort) require a new order ID after cancellation.'),
  );
  $form['general']['hmac'] = array(
    '#type' => 'select',
    '#title' => t('Perform HMAC control'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'hmac', ($default ? 1 : '')),
    '#description' => t('If enabled, DIBS will make some extra checks on the sended data, to be sure that no one manipulated it. If enabled should the keys below be filled in!'),
  );
  $form['general']['hmac_key'] = array(
    '#type' => 'textarea',
    '#resizable' => FALSE,
    '#rows' => 2,
    '#title' => t('HMAC key'),
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'hmac_key', ''),
    '#description' => t('HMAC key - used if "Perform HMAC control" is enabled.'),
  );
  
  $form['general']['md5'] = array(
    '#type' => 'select',
    '#title' => t('Perform MD5 control'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'md5', ($default ? 1 : '')),
    '#description' => t('If enabled, DIBS will make some extra checks on the sended data, to be sure that no one manipulated it. If enabled should the keys below be filled in!'),
  );
  $form['general']['md5_key1'] = array(
    '#type' => 'textfield',
    '#title' => t('MD5 key 1'),
    '#maxlength' => 64,
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'md5_key1', ''),
    '#description' => t('MD5 key 1 - used if "Perform MD5 control" is enabled.'),
  );
  $form['general']['md5_key2'] = array(
    '#type' => 'textfield',
    '#title' => t('MD5 key 2'),
    '#maxlength' => 64,
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'md5_key2', ''),
    '#description' => t('MD5 key 2 - used if "Perform MD5 control" is enabled.'),
  );

  $form['general']['lang'] = array(
    '#type' => 'select',
    '#title' => t('Language'),
    '#options' => array(
                        '' => '',
                        'da' => 'Danish',
                        'sv' => 'Swedish',
                        'no' => 'Norwegian',
                        'en' => 'English',
                        'nl' => 'Dutch',
                        'de' => 'German',
                        'fr' => 'French',
                        'fi' => 'Finnish',
                        'es' => 'Spanish',
                        'it' => 'Italian',
                        'pl' => 'Polish'
                      ),
    '#required' => $default,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'lang', ''),
    '#description' => t('Language code for the language used on the DIBS payment window'),
  );
  $form['general']['currency'] = array(
    '#type' => 'select',
    '#title' => t('Currency'),
    '#options' => array(
                        '' => '',
                        '208' => 'Danish Kroner (DKK)',
                        '978' => 'Euro (EUR)',
                        '840' => 'US Dollar $ (USD)',
                        '826' => 'English Pound £ (GBP)',
                        '752' => 'Swedish Kronor (SEK)',
                        '036' => 'Australian Dollar (AUD',
                        '124' => 'Canadian Dollar (CAD)',
                        '352' => 'Icelandic Króna (ISK)',
                        '392' => 'Japanese Yen (JPY)',
                        '554' => 'New Zealand Dollar (NZD)',
                        '578' => 'Norwegian Kroner (NOK)',
                        '756' => 'Swiss Franc (CHF)',
                        '949' => 'Turkish Lire (TRY)',
                      ),
    '#required' => $default,
    '#default_value' => dibs_get_settings_value($module, $delta, 'general', 'currency', ''),
    '#description' => t('Currency code for the currency used when paying.'),
  );

  // Payment window
  $form['paymentwindow'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payment Window settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );

  _dibs_payment_types_element($form, 'paymentwindow', $module, $delta, DIBS_PAYMENT_WINDOW); 
  
  $form['paymentwindow']['color'] = array(
    '#type' => 'select',
    '#title' => t('Color theme'),
    '#options' => array(
                        '' => '',
                        'sand' => t('Sand'),
                        'grey' => t('Grey'),
                        'blue' => t('Blue'),
                      ),
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'paymentwindow', 'color', ''),
    '#description' => t('The color theme for the DIBS popup window.'),
  );

  // Flex window
  $form['flexwindow'] = array(
    '#type' => 'fieldset',
    '#title' => t('Flex Window settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );

  _dibs_payment_types_element($form, 'flexwindow', $module, $delta, DIBS_FLEXWIN); 

  $form['flexwindow']['color'] = array(
    '#type' => 'select',
    '#title' => t('Color theme'),
    '#options' => array(
                        '' => '',
                        'sand' => t('Sand'),
                        'grey' => t('Grey'),
                        'blue' => t('Blue'),
                      ),
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'flexwindow', 'color', ''),
    '#description' => t('The color theme for the DIBS popup window.'),
  );
  $form['flexwindow']['decorator'] = array(
    '#type' => 'select',
    '#title' => t('Decorator'),
    '#options' => array(
                        '' => '',
                        'default' => t('Default'),
                        'basal' => t('Basal'),
                        'rich' => t('Rich'),
                        'custom' => t('Custom'),
                      ),
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'flexwindow', 'decorator', ''),
    '#description' => t('Choose what DIBS decorator to use. If you want to use the one configured in the DIBS administration, please then choose "Custom".'),
  );
  $form['flexwindow']['voucher'] = array(
    '#type' => 'select',
    '#title' => t('Voucher'),
    '#options' => array(
                        '' => '',
                        0 => t('No'),
                        1 => t('Yes'),
                      ),
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'flexwindow', 'voucher', ''),
    '#description' => t('If set to Yes, then the list of payment types on the first page of FlexWin will contain vouchers, too.'),
  );

  // Order ID
  $form['orderid'] = array(
    '#type' => 'fieldset',
    '#title' => t('Order ID settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['orderid']['order_id_old'] = array(
    '#type' => 'value',
    '#value' => !empty($settings['orderid']['order_id']) ? $settings['orderid']['order_id'] : '',
  );
  $form['orderid']['order_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Next order ID'),
    '#maxlength' => 10,
    '#legnth' => 10,
    '#required' => FALSE,
    '#default_value' => !empty($settings['orderid']['order_id']) ? $settings['orderid']['order_id'] : '0',
    '#description' => t('The next order ID value. Only numeric values are allowed. Please besure that the range used does not conflict with order IDs on existing payments in DIBS, and other implementations of the DIBS module.'),
  );
  $form['orderid']['order_id_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Order ID prefix'),
    '#maxlength' => 10,
    '#legnth' => 10,
    '#required' => FALSE,
    '#default_value' => !empty($settings['orderid']['order_id_prefix']) ? $settings['orderid']['order_id_prefix'] : '',
    '#description' => t('Prefix used before the order ID value, when the payment is sent to DIBS. Using the prefix and/or suffix makes it easier to control that the order ID is unique when sent to DIBS. Characters allowed: A-z0-9-_'),
  );
  $form['orderid']['order_id_suffix'] = array(
    '#type' => 'textfield',
    '#title' => t('Order ID suffix'),
    '#maxlength' => 10,
    '#legnth' => 10,
    '#required' => FALSE,
    '#default_value' => !empty($settings['orderid']['order_id_suffix']) ? $settings['orderid']['order_id_suffix'] : '',
    '#description' => t('Suffix used after the order ID value, when the payment is sent to DIBS. Using the prefix and/or suffix makes it easier to control that the order ID is unique when sent to DIBS. Characters allowed: A-z0-9-_'),
  );

  // Custom URL callbacks
  $form['callbacks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Callback URLs'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['callbacks']['accepturl'] = array(
    '#type' => 'textfield',
    '#title' => t('Accept URL'),
    '#maxlength' => 255,
    '#legnth' => 50,
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'callbacks', 'accepturl', 'payment/dibs/accept'),
    '#description' => t('The URL that DIBS should call after a transacation has been accepted.'),
  );
  $form['callbacks']['cancelurl'] = array(
    '#type' => 'textfield',
    '#title' => t('Cancel URL'),
    '#maxlength' => 255,
    '#legnth' => 50,
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'callbacks', 'cancelurl', 'payment/dibs/cancel'),
    '#description' => t('The URL that DIBS should call after a transaction has been canceled by the user.'),
  );
  $form['callbacks']['callbackok'] = array(
    '#type' => 'textfield',
    '#title' => t('OK URL'),
    '#maxlength' => 255,
    '#legnth' => 50,
    '#required' => FALSE,
    '#default_value' => dibs_get_settings_value($module, $delta, 'callbacks', 'callbackok', 'payment/dibs/callbackok'),
    '#description' => t('The URL that DIBS should call to validate that a transaction is OK.'),
  );

  // Advanced settings
  $form['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['advanced']['calcfee'] = array(
    '#type' => 'select',
    '#title' => t('Calc fee'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'advanced', 'calcfee', ($default ? 0 : '')),
    '#description' => t('If enabled, the Payment Window will automatically affix the charge due to the transaction, i.e. the charge payable to the acquirer (e.g. PBS), and display this to the customer.'),
  );
  $form['advanced']['capturenow'] = array(
    '#type' => 'select',
    '#title' => t('Capture now'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'advanced', 'capturenow', ($default ? 0 : '')),
    '#description' => t('If enabled, the amount is immediately transferred from the customers account to the shop\'s account. This function can only be utilized in the event that there is no actual physical delivery of any items.'),
  );
  $form['advanced']['unqiue_order_id'] = array(
    '#type' => 'select',
    '#title' => t('Unique order ID'),
    '#options' => array('' => '', 0 => t('No'), 1 => t('Yes')),
    '#default_value' => dibs_get_settings_value($module, $delta, 'advanced', 'unqiue_order_id', ($default ? 1 : '')),
    '#description' => t('If enabled, the order ID must be unique, i.e. there is no existing transaction with DIBS with the same order ID. If such a transaction already exists, payment will be rejected with reason=7.'),
  );

  $form['module'] = array(
    '#type' => 'value',
    '#value' => $module,
  );
  $form['delta'] = array(
    '#type' => 'value',
    '#value' => $delta,
  );
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save settings'));

  return $form;
}

/**
 * Validate settings form
 *
 * @param array $form
 * @param array $form_state
 */
function dibs_admin_settings_form_validate($form, &$form_state) {
  $module = $form_state['values']['module'];
  $delta  = $form_state['values']['delta'];

  // Checking if the module and delta exists
  $info = module_invoke($module, 'dibsapi', 'info', $delta);
  if (empty($info) || !is_array($info) || empty($module) || empty($delta)) {
    drupal_not_found();
    exit;
  }

  // Checking if module and delta are default
  $default = dibs_is_default($module, $delta);

  // Checking if keys are filled in when MD5 check is enabled
  if ($form_state['values']['general']['md5']) {
    if (empty($form_state['values']['general']['md5_key1'])) {
      form_set_error('general][md5_key1', t('Please, fill in the missing key, or disable "Perform MD5 control". The MD5 keys can be found in the DIBS administration.'));
    }
    if (empty($form_state['values']['general']['md5_key2'])) {
      form_set_error('general][md5_key2', t('Please, fill in the missing key, or disable "Perform MD5 control". The MD5 keys can be found in the DIBS administration.'));
    }
  }

  // Checking if key is filled in when HMAC check is enabled
  if ($form_state['values']['general']['hmac']) {
    if (empty($form_state['values']['general']['hmac_key'])) {
      form_set_error('general][hmac_key', t('Please, fill in the missing key, or disable "Perform HMAC control". The HMAC key can be found in the DIBS administration.'));
    }
  }
  
  // Checking order ID settings
  if ($form_state['values']['orderid']['order_id'] == '') {
    form_set_error('orderid][order_id', t('Please, fill in the next order ID.'));
  }
  elseif (!empty($form_state['values']['orderid']['order_id']) && !is_numeric($form_state['values']['orderid']['order_id'])) {
    form_set_error('orderid][order_id', t('The order ID should be numeric.'));
  }
  
  // Checking that suffix and prefix are valid
  if (!empty($form_state['values']['orderid']['order_id_prefix']) && !preg_match("/^[A-z0-9\-_]+$/", $form_state['values']['orderid']['order_id_prefix'])) {
    form_set_error('orderid][order_id_prefix', t('The order id prefix contains not valid characters. Characters allowed in the field: A-z0-9_-'));
  }
  if (!empty($form_state['values']['orderid']['order_id_suffix']) && !preg_match("/^[A-z0-9\-_]+$/", $form_state['values']['orderid']['order_id_suffix'])) {
    form_set_error('orderid][order_id_suffix', t('The order id suffix contains not valid characters. Characters allowed in the field: A-z0-9_-'));
  }
}

/**
 * Process the settings form submit
 *
 * @param array $form
 * @param array $form_state
 */
function dibs_admin_settings_form_submit($form, &$form_state) {
  $module         = $form_state['values']['module'];
  $delta          = $form_state['values']['delta'];
  $general        = $form_state['values']['general'];
  $paymentwindow  = $form_state['values']['paymentwindow'];
  $flexwindow     = $form_state['values']['flexwindow'];
  $orderid        = $form_state['values']['orderid'];
  $advanced       = $form_state['values']['advanced'];
  $callbacks      = $form_state['values']['callbacks'];

  // Checking if the module and delta exists
  $info = module_invoke($module, 'dibsapi', 'info', $delta);
  if (empty($info) || !is_array($info) || empty($module) || empty($delta)) {
    drupal_not_found();
    exit;
  }

  $paymentwindow['paytypes'] = array();
  foreach(array_keys(dibs_paytypes_types(DIBS_PAYMENT_WINDOW)) as $key) {
    $paymentwindow['payment_types'][$key]['types'] = array_filter($paymentwindow['payment_types'][$key]['types']);
    if($paymentwindow['payment_types'][$key]['selection'] === 'select') {
      $paymentwindow['paytypes'] = array_merge($paymentwindow['paytypes'], array_values($paymentwindow['payment_types'][$key]['types']));
    }
    elseif (!empty($paymentwindow['payment_types'][$key]['selection'])) {
      $paymentwindow['paytypes'][] = $paymentwindow['payment_types'][$key]['selection'];
    }
  }

  $flexwindow['paytypes'] = array();
  foreach(array_keys(dibs_paytypes_types(DIBS_FLEXWIN)) as $key) {
    $flexwindow['payment_types'][$key]['types'] = array_filter($flexwindow['payment_types'][$key]['types']);
    if($flexwindow['payment_types'][$key]['selection'] === 'select') {
      $flexwindow['paytypes'] = array_merge($flexwindow['paytypes'], array_values($flexwindow['payment_types'][$key]['types']));
    }
    elseif (!empty($flexwindow['payment_types'][$key]['selection'])) {
      $flexwindow['paytypes'][] = $flexwindow['payment_types'][$key]['selection'];
    }
  }

  // Creating array with settings and saving it as a variable
  $settings = array(
    'general' => $general,
    'paymentwindow' => $paymentwindow,
    'flexwindow' => $flexwindow,
    'advanced' => $advanced,
    'callbacks' => $callbacks,
  );
  variable_set('dibs_settings_' . $module . '_' . $delta, $settings);

  // Saving order ID settings
  $result = db_query('
    SELECT api_module 
    FROM {dibs_order_id_settings}
    WHERE api_module = :api_module
    AND api_delta = :api_delta',
    array(':api_module' => $module, ':api_delta' => $delta,)
  );

  if ($result->rowCount()) {
    //  $orderid['order_id'], $orderid['order_id_suffix'], $orderid['order_id_prefix'], $module, $delta);
    $updated = db_update('dibs_order_id_settings')
      ->fields(array(
        'order_id' => $orderid['order_id'],
        'order_id_suffix' => $orderid['order_id_suffix'],
        'order_id_prefix' => $orderid['order_id_prefix'],
      ))
      ->condition('api_module', $module)
      ->condition('api_delta', $delta)
      ->execute();
  }
  else {
    //  $module, $delta, $orderid['order_id'], $orderid['order_id_prefix'], $orderid['order_id_suffix']);
    $nid = db_insert('dibs_order_id_settings')
    ->fields(array(
      'api_module' => $module,
      'api_delta' => $delta,
      'order_id' => $orderid['order_id'],
      'order_id_prefix' => $orderid['order_id_prefix'],
      'order_id_suffix' => $orderid['order_id_suffix'],
      ))
    ->execute();
  }

  drupal_get_messages();
  drupal_set_message(t('The settings are now saved.'));
 
  $form_state['redirect'] = 'admin/config/payment/dibs';
}

function _dibs_payment_types_element(&$element, $element_name, $module, $delta, $paywin_type) {

  //'#default_value' => dibs_get_settings_value($module, $delta, 'advanced', 'unqiue_order_id', ($default ? 1 : '')),
  $settings = dibs_get_settings_value($module, $delta, $element_name, 'payment_types', array());

  $element[$element_name]['payment_types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payment types'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    //'#description' => 'TODO',
  );

  $element = &$element[$element_name]['payment_types'];

  foreach(dibs_paytypes_types($paywin_type) as $type => $info) {
    $element[$type] = array(
      '#title' => $info['label'],
      '#type' => 'fieldset',
    );
    $element[$type]['selection'] = array(
      '#title' => t('Selection'),
      '#type' => 'select',
      '#options' => array(
        '' => t('None'),
        $info['select_all'] => t('All'),
        'select' => t('Select'),
      ),
      '#default_value' => isset($settings[$type]['selection']) ? $settings[$type]['selection'] : '',
    );
    $selection_selector = 'select[name="' . $element_name . '[' . implode('][', array('payment_types', $type, 'selection')) . ']"]';

    $element[$type]['types'] = array(
      '#title' => t('Select types'),
      '#type' => 'checkboxes',
      '#states' => array(
        'visible' => array(
          $selection_selector => array('value' => 'select'),
        ),
      ),
      '#options' => $info['types'],
      '#default_value' => isset($settings[$type]['types']) ? $settings[$type]['types'] : NULL,
    );
  }
}

function dibs_paytypes_types($paywin_type) {
  return array(
    'cards' => array(
      'label' => t('Cards'),
      'types' => dibs_paytypes_cards($paywin_type),
      'select_all' => 'ALL_CARDS',
    ),
    'netbanks' => array(
      'label' => t('Netbanks'),
      'types' => dibs_paytypes_netbanks($paywin_type),
      'select_all' => 'ALL_NETBANKS',
    ),
    'invoices' => array(
      'label' => t('Invoices'),
      'types' => dibs_paytypes_invoices($paywin_type),
      'select_all' => 'ALL_INVOICES',
    ),
  );
}

function dibs_paytypes_cards($paywin_type) {
  switch($paywin_type) {
    case DIBS_FLEXWIN:
    case DIBS_PAYMENT_WINDOW:
      return array(
        'AAK' => t('Århus city kort'),
        'ACCEPT' => t('Accept card'),
        'ACK' => t('Albertslund Centrum Kundekort'),
        'AKK' => t('Apollo-/Kuonikonto'),
        'AMEX' => t('American Express'),
        'AMEX(DK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('American Express (Danish card)'), '!parent_cardtype' => t('AMEX'))),
        'AMEX(SE)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('American Express (Swedishcard)'), '!parent_cardtype' => t('AMEX'))),
        'BHBC' => t('Bauhaus Best card'),
        'CCK' => t('Computer City Customer Card'),
        'DAELLS' => t('Daells Bolighus Kundekort'),
        'DIN' => t('Diners Club'),
        'DIN(DK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Diners Club (Danish card)'), '!parent_cardtype' => t('DIN'))),
        'DK' => t('Dankort'),
        'ELEC' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('VISA Electron'), '!parent_cardtype' => t('VISA'))),
        'EWORLD' => t('Electronic World Credit Card'),
        'FCC' => t('Ford Credit Card'),
        'FCK' => t('Frederiksberg Centret Kundekort'),
        'FFK' => t('Forbrugsforeningen Card'),
        'FINX(SE)' => t('Finax (SE)'),
        'FISC' => t('Fields Shoppingcard'),
        'FLEGCARD' => t('Fleggard kort'),
        'FSC' => t('Fisketorvet Shopping Card'),
        'GIT' => t('Getitcard'),
        'GSC' => t('Glostrup Shopping Card'),
        'GRA' => t('Graphium'),
        'HEME' => t('Hemtex faktura'),
        'HEMP' => t('Hemtex personalkort'),
        'HEMTX' => t('Hemtex clubkort'),
        'HMK' => t('HM Konto (Hennes og Mauritz)'),
        'HNYBORG' => t('Harald Nyborg'),
        'HSC' => t('Hillerød Shopping Card'),
        'HTX' => t('Hydro Texaco'),
        'IBC' => t('Inspiration Best Card'),
        'IKEA' => t('IKEA kort'),
        'ISHBY' => t('Sparbank Vestkort'),
        'JCB' => t('JCB (Japan Credit Bureau)'),
        'JEM_FIX' => t('Jem&Fix Kundekort'),
        'KAUPBK' => t('Kaupthing Bankkort'),
        'LFBBK' => t('LänsförsäkringarBank Bankkort'),
        'LIC(DK)' => t('Lærernes IndkøbsCentral (Denmark)'),
        'LIC(SE)' => t('Lærernes IndkøbsCentral (Sweden)'),
        'LOPLUS' => t('LO Plus Guldkort'),
        'MC' => t('Mastercard'),
        'MC(DK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Mastercard (Danish card)'), '!parent_cardtype' => t('MC'))),
        'MC(SE)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Mastercard (Swedish card)'), '!parent_cardtype' => t('MC'))),
        'MC(YX)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('YX Mastercard'), '!parent_cardtype' => t('MC'))),
        'MEDM' => t('Medmera'),
        'MERLIN' => t('Merlin Kreditkort'),
        'MTRO' => t('Maestro'),
        'MTRO(DK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Maestro (DK)'), '!parent_cardtype' => t('MTRO'))),
        'MTRO(UK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Maestro (UK)'), '!parent_cardtype' => t('MTRO'))),
        'MTRO(SOLO)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Solo'), '!parent_cardtype' => t('MTRO'))),
        'MTRO(SE)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Maestro (SE)'), '!parent_cardtype' => t('MTRO'))),
        'MYHC' => t('My Holiday Card'),
        'NSBK' => t('Nordea Bankkort'),
        'OESBK' => t('Östgöta EnskildaBankkort'),
        'PayPal' => t('Paypal'),
        'Q8SK' => t('Q8 ServiceKort'),
        'REB' => t('Resurs Bank'),
        'REMCARD' => t('Remember Card'),
        'ROEDCEN' => t('Rødovre Centerkort'),
        'S/T' => t('Spies/Tjæreborg'),
        'SBSBK' => t('Skandiabanken Bankkort'),
        'SEB_KOBK' => t('SEB Köpkort'),
        'SEBSBK' => t('SEB Bankkort'),
        'SHB' => t('HandelsbankenKöpkort'),
        'SILV_ERHV' => t('Silvan Konto Erhverv'),
        'SILV_PRIV' => t('Silvan Konto Privat'),
        'STARTOUR' => t('Star Tour'),
        'TLK' => t('Tæppeland'),
        'TUBC' => t('Toys R Us - BestCard'),
        'V-DK' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('VISA-Dankort'), '!parent_cardtype' => t('DK'))),
        'VEKO' => t('VEKO Finans'),
        'VISA' => t('VISA'),
        'VISA(DK)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('VISA (DK)'), '!parent_cardtype' => t('VISA'))),
        'VISA(SE)' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('VISA (SE)'), '!parent_cardtype' => t('VISA'))),
        'WOCO' => t('!cardtype_description, parent cardtype: !parent_cardtype', array('!cardtype_description' => t('Wonderful Copenhagen Card'), '!parent_cardtype' => t('VISA'))),
      );
    default:
      throw new Exception();
  }
}

function dibs_paytypes_netbanks($paywin_type) {
  switch($paywin_type) {
    case DIBS_FLEXWIN:
      return _dibs_flexwin_paytypes_netbanks();

    case DIBS_PAYMENT_WINDOW:
      return _dibs_paymentwindow_paytypes_netbanks();

    default:
      throw new Exception();
  }
}

function _dibs_flexwin_paytypes_netbanks() {
  return array(
    'AAL' => t('Ålandsbanken'),
    'ABN' => t('ABN AMRO iDeal Payment'),
    'AKTIA or AKT_A' => t('Aktia Web Payment'),
    'BAX' => t('BankAxess'),
    'CC' => t('CoinClick'),
    'DNB' => t('Danske Netbetaling (Danske Bank)'),
    'ECRED' => t('eCredit Payment'),
    'EDK' => t('eDankort'),
    'ELV' => t('Bank Einzug (eOLV)'),
    'SWD_A' => t('Swedbank Direktbetalning'),
    'HNS' => t('HansaBank'),
    'ING' => t('iDeal Web Payment (ING Bank)'),
    'NDB2' => t('Nordea Solo-E betaling (Sweden)'),
    'OKO' => t('OKO Web Payment'),
    'P24_A' => t('Przelewy24'),
    'PAGOC' => t('PagoCard'),
    'RBS' => t('Royal Bank of Scotland iDeal Payment'),
    'SAMPO' => t('Sampo Web Payment'),
    'SEB_A' => t('SEB Direktbetalning'),
    'SHB_A' => t('SHB Direktbetalning'),
    'SOLO' => t('Nordea Solo-E betaling (Denmark)'),
    'SOLOFI' => t('Nordea Solo-E payment (Finland)'),
    'TAP' => t('Tapiola'),
  );
}
function _dibs_paymentwindow_paytypes_netbanks() {
  return array(
    'AAL' => t('Ålandsbanken'),
    'AKT_A' => t('Aktia Web Payment'),
    'swd_a' => t('Swedbank Direktbetalning'),
    'NDB2' => t('Nordea Solo-E betaling (Sweden)'),
    'P24_A' => t('Przelewy24'),
    'seb_a' => t('SEB Direktbetalning'),
    'shb_a' => t('SHB Direktbetalning'),
    'TAP' => t('Tapiola'),
  );
}

function dibs_paytypes_invoices($paywin_type) {
  switch($paywin_type) {
    case DIBS_FLEXWIN:
    case DIBS_PAYMENT_WINDOW:
      return array(
        'pbb' => t('PayByBill'),
        'pbbtest' => t('PayByBill Test'),
        'cl_inv' => t('Collector PayPort'),
        'cl_invtest' => t('Collector PayPort Test'),
        'cl_ins' => t('Collector PayPort Installment'),
        'cl_instest' => t('Collector PayPort Installment Test'),
        'sv_inv' => t('SveaWebPay'),
        'sv_invtest' => t('SveaWebPay Test'),
        'sv_ins' => t('SveaWebPay Installments'),
        'sv_instest' => t('SveaWebPay Installments Test'),
        'kl_inv' => t('Klarna'),
        'kl_invtest' => t('Klarna Test'),
        'kl_ins' => t('Klarna Installments'),
        'kl_instest' => t('Klarna Installments Test'),
        'shb_inv' => t('Handelsbanken Finans'),
        'shbinvtest' => t('Handelsbanken Finans Test'),
      );
    
    default:
      throw new Exception();
  }
}
