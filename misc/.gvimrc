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

function! Build()
  terminal ++curwin

  if g:os ==# 'Windows'
    let g:build_cmd = findfile("windows-build.bat", ".;")
  elseif g:os ==# 'Linux'
    let g:build_cmd = findfile("linux-build.sh", ".;")
  else
    let g:build_cmd = findfile("mac-build.sh", ".;")
  endif   

  call term_sendkeys(bufnr("%"), g:build_cmd . "\<CR>")
endfunction
nnoremap <C-B> :silent! call Build()<CR>

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
