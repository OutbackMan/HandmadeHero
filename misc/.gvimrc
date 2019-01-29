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

if has("gui_gtk3")
  set guifont=Inconsolata\ 12
elseif has("gui_macvim")
  set guifont=Menlo\ Regular:14
else
  set guifont=Consolas:h11:cANSI
endif

set cc=80
set showmatch
set backspace=indent,eol,start
set laststatus=2
set showmode
set scrolloff=5
set nowrap


if !exists('g:os')
  if has('win32') || has('win64')
    let g:os = 'Windows'
    autocmd GUIEnter * simalt ~x
  else
    let g:os = substitute(system('uname'), '\n', '', '')
  endif
endif

tnoremap <silent> <Esc> <C-W>:
function! Build()
  tabedit __BUILD__
  terminal ++curwin

  " NOTE(Ryan): Could not get cross-platform findfile() working
  " So, just have to :cd into root directory
  if g:os ==# 'Windows'
    let g:build_cmd = "windows-build.bat" 
  elseif g:os ==# 'Linux'
    let g:build_cmd = "bash linux-build.bash"
  else
    let g:build_cmd = "bash mac-build.bash"
  endif   

  call term_sendkeys(bufnr("%"), g:build_cmd . "\<CR>")
endfunction
nnoremap <silent> <C-B> :call Build()<CR>
" NOTE(Ryan): Had cross-platform issues trying to close tab with single
" command
tnoremap <silent> <C-Tab> exit<CR><C-W>:tabclose!<CR>:execute "bdelete! " . bufnr("$")<CR>
" TODO(Ryan): Strive to use codeclap when not so buggy
" tnoremap <silent> <C-CR> cdtdebug.sh -e build/linux-hh<CR>


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
