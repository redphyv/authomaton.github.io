(function () {
  const links = [
    { label: 'Certifications', href: 'certifications.html' },
    { label: 'Homelab',        href: 'homelab.html' },
    { label: 'Project Gamma',  href: '#' },
    { label: 'Project Delta',  href: '#' },
  ];

  const currentFile = window.location.pathname.split('/').pop() || 'index.html';

  const items = links.map(({ label, href }) => {
    const isActive = href === currentFile;
    const active = isActive ? ' aria-current="page" class="sidebar-link active"' : ' class="sidebar-link"';
    const prefix = isActive ? '# ' : '> ';
    return `<li><a href="${href}"${active}>${prefix}${label}</a></li>`;
  }).join('\n            ');

  const html = `
    <nav class="sidebar-nav">
      <h2 class="sidebar-heading">Projects</h2>
      <ul class="sidebar-links list-unstyled">
            ${items}
      </ul>
    </nav>`;

  const placeholder = document.getElementById('sidebar-placeholder');
  if (placeholder) placeholder.innerHTML = html;
})();
