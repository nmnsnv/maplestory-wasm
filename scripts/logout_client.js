(() => {
  const logout = window.MapleWasmUI?.logout || Module?._maple_logout;

  if (typeof logout !== 'function') {
    console.warn('[logout] Client logout hook is not available on this page.');
    return false;
  }

  logout();
  console.log('[logout] Returned to the login screen.');
  return true;
})();
