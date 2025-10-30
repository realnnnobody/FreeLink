(function(){
  const msgs = document.getElementById('msgs');
  const input = document.getElementById('input');
  const btn = document.getElementById('send');
  const devip = document.getElementById('devip');
  devip.textContent = location.hostname;

  const ws = new WebSocket('ws://' + location.hostname + ':81/');
  ws.onopen = ()=> console.log('ws open');
  ws.onclose = ()=> console.log('ws close');
  ws.onerror = e => console.warn('ws err', e);

  function addMessage(text, who){
    const el = document.createElement('div');
    el.className = 'msg ' + (who==='me' ? 'me' : 'other');
    el.textContent = text;
    msgs.appendChild(el);
    msgs.scrollTop = msgs.scrollHeight;
  }

  ws.onmessage = (ev)=>{
    addMessage(ev.data, 'other');
  }

  function send(){
    const v = input.value.trim();
    if(!v) return;
    addMessage(v, 'me');
    ws.send(v);
    input.value='';
    input.focus();
  }

  btn.addEventListener('click', send);
  input.addEventListener('keydown', e=>{ if(e.key==='Enter') send(); });
})();