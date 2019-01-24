set nocompatible

set encoding=iso-8859-2
set noswapfile noundofile nobackup

set expandtab softtabstop=2 tabstop=2
set shiftwidth=2 shiftround 
set autoindent autowrite

set mouse=a

set incsearch hlsearch 

set foldenable
set foldmethod=manual
set foldlevelstart=0

syntax on
set number
set guifont=Monospace\ 16
set cc=80
set showmatch
set backspace=indent,eol,start
set laststatus=2
set showmode
set scrolloff=5
set nowrap


if !exists('g:os')
  if has ('win32') || has('win64')
    let g:os = 'Windows'
    autocmd GUIEnter * simalt ~x
  else
    let g:os = substitute(system('uname'), '\n', '', '')
    if g:os ==# 'Linux'
      autocmd GUIEnter * call system('wmctrl -i -b add,maximized_vert,maximized_horz -r' . v:windowid)
    endif
  endif
endif

autocmd VimEnter * call VSplitWindow()  

function! VSplitWindow()
  let g:left_vsplit_buf_num = bufnr("%")     
  silent! vsplit __SCRATCH__
  let g:right_vsplit_buf_num = bufnr("%")
  silent! wincmd L
  silent! wincmd h
  let g:using_default_vsplit_bufs = 1
  let g:left_vsplit_buf_is_active = 0
endfunction

function! MoveToOtherVSplitWindow()
  if g:left_vsplit_buf_is_active
    silent! wincmd l
    let g:left_vsplit_buf_is_active = 0
  else
    silent! wincmd h
    let g:left_vsplit_buf_is_active = 1
  endif
endfunction

nnoremap <C-Space> silent! call MoveToOtherVSplitWindow()
tnoremap <Esc> <C-W>:

command! -nargs=1 -complete=file FF call OpenFileInsideInactiveSplit(<f-args>)

function! OpenFileInsideInactiveSplit(file_name)
  " NOTE(Ryan): On first file opening, want to stay on left buf
  if g:using_default_vsplit_bufs
    silent! execute "edit " . a:file_name 
    " NOTE(Ryan): New buf will take same num as default buf. No need to update
    let g:using_default_vsplit_bufs = 0
    let g:left_vsplit_buf_is_active = 1
    return
  endif
  " NOTE(Ryan): Will retain vsplit across movements if file saved
  if g:left_vsplit_buf_is_active
    silent! wincmd l
    silent! execute "edit " . a:file_name 
    if g:right_vsplit_buf_num != g:left_vsplit_buf_num
      silent! execute "bdelete! " . g:right_vsplit_buf_num
    endif
    let g:right_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 0
  else
    silent! wincmd h
    silent! execute "edit " . a:file_name 
    if g:left_vsplit_buf_num != g:right_vsplit_buf_num
      silent! execute "bdelete! " . g:left_vsplit_buf_num
    endif
    let g:left_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 1
  endif 
endfunction

nnoremap <C-B> :silent! call Build()<CR>

function! Build()
  if g:left_vsplit_buf_is_active
    silent! wincmd l
    terminal ++curwin
    let g:right_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 0
  else
    silent! wincmd h
    terminal ++curwin
    let g:left_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 1
  endif 
  " NOTE(Ryan): never manually close window
  " NOTE(Ryan): inside terminal, use ctrl-w
  
  if g:os ==# 'Windows'
    let g:build_cmd = findfile("windows-build.bat", ".;")
  else if g:os ==# 'Linux'
    let g:build_cmd = findfile("linux-build.sh", ".;")
  else
    let g:build_cmd = findfile("mac-build.sh", ".;")
  endif   

  call term_sendkeys(bufnr("%"), g:build_cmd . "\<CR>")
endfunction

" NOTE(Ryan): <C-R><C-W> to insert current word
command! -nargs=1 FW call VimgrepQuickfixInsideInactiveSplit(<f-args>)
function! VimgrepQuickfixInsideInactiveSplit(search_str)
  if g:left_vsplit_buf_is_active
    silent! wincmd l
    let g:in_opened_quickfix = 1
    silent! execute 'vimgrep /' . a:search_str . '/g **/*.[chm]' 
    if g:right_vsplit_buf_num != g:left_vsplit_buf_num
      silent! execute "bdelete! " . g:right_vsplit_buf_num
    endif
    let g:right_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 0
  else
    silent! wincmd h
    let g:in_opened_quickfix = 1
    silent! execute 'vimgrep /' . a:search_str . '/g **/*.[ch]'  
    if g:left_vsplit_buf_num != g:right_vsplit_buf_num
      silent! execute "bdelete! " . g:left_vsplit_buf_num
    endif
    let g:left_vsplit_buf_num = bufnr("%")
    let g:left_vsplit_buf_is_active = 1
  endif 
endfunction
nnoremap n g:in_opened_quickfix ? :cn<CR> : n

function! TabSelectOrPopupOrIndent()
  if col('.') == 1 || getline('.')[col('.') - 2] =~? '[ ]'
    return "\<Tab>"
  else
    return "\<C-X>\<C-N>"
  endif
endfunction
inoremap <expr> <Tab> TabSelectOrPopupOrIndent()
inoremap <expr> <CR> pumvisible() ? "\<C-Y>" : "\<CR>"
inoremap <expr> <Esc> pumvisible() ? "\<C-E>" : "\<Esc>"
inoremap <expr> n pumvisible() ? "\<C-N>" : 'n'
inoremap <expr> <S-N> pumvisible() ? "\<C-P>" : "\<S-N>"
